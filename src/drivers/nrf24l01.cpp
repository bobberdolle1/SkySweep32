#include "nrf24l01.h"

#ifdef MODULE_NRF24

NRF24L01Driver::NRF24L01Driver(uint8_t csPin, uint8_t cePin, SPIClass* spi) {
    chipSelectPin = csPin;
    chipEnablePin = cePin;
    spiInstance = spi;
    currentChannel = 76; // Default 2.476 GHz
    initialized = false;
}

bool NRF24L01Driver::begin() {
    pinMode(chipSelectPin, OUTPUT);
    pinMode(chipEnablePin, OUTPUT);
    digitalWrite(chipSelectPin, HIGH);
    digitalWrite(chipEnablePin, LOW);
    
    delay(100);
    
    // Power up and configure
    powerUp();
    delay(5);
    
    // Verify connection
    writeRegister(NRF24_SETUP_AW, 0x03); // 5-byte address width
    uint8_t test = readRegister(NRF24_SETUP_AW);
    if (test != 0x03) {
        Serial.println("[NRF24] Chip not detected");
        return false;
    }
    
    // Configuration
    writeRegister(NRF24_CONFIG, 0x0E); // PWR_UP, EN_CRC, 2-byte CRC
    writeRegister(NRF24_EN_AA, 0x00);  // Disable auto-ack for promiscuous mode
    writeRegister(NRF24_EN_RXADDR, 0x01); // Enable RX pipe 0
    writeRegister(NRF24_SETUP_RETR, 0x00); // No retransmit
    setChannel(76);
    setDataRate(2); // 2 Mbps
    setPowerLevel(3); // Max power
    setPayloadSize(32);
    
    flushRx();
    flushTx();
    clearInterrupts();
    
    initialized = true;
    setRxMode();
    
    Serial.println("[NRF24] Initialized successfully");
    return true;
}

void NRF24L01Driver::writeRegister(uint8_t reg, uint8_t value) {
    digitalWrite(chipSelectPin, LOW);
    spiInstance->transfer(NRF24_W_REGISTER | (reg & 0x1F));
    spiInstance->transfer(value);
    digitalWrite(chipSelectPin, HIGH);
}

uint8_t NRF24L01Driver::readRegister(uint8_t reg) {
    digitalWrite(chipSelectPin, LOW);
    spiInstance->transfer(NRF24_R_REGISTER | (reg & 0x1F));
    uint8_t value = spiInstance->transfer(0xFF);
    digitalWrite(chipSelectPin, HIGH);
    return value;
}

void NRF24L01Driver::writeRegisterMulti(uint8_t reg, uint8_t* data, uint8_t length) {
    digitalWrite(chipSelectPin, LOW);
    spiInstance->transfer(NRF24_W_REGISTER | (reg & 0x1F));
    for (uint8_t i = 0; i < length; i++) {
        spiInstance->transfer(data[i]);
    }
    digitalWrite(chipSelectPin, HIGH);
}

void NRF24L01Driver::readRegisterMulti(uint8_t reg, uint8_t* buffer, uint8_t length) {
    digitalWrite(chipSelectPin, LOW);
    spiInstance->transfer(NRF24_R_REGISTER | (reg & 0x1F));
    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = spiInstance->transfer(0xFF);
    }
    digitalWrite(chipSelectPin, HIGH);
}

uint8_t NRF24L01Driver::sendCommand(uint8_t command) {
    digitalWrite(chipSelectPin, LOW);
    uint8_t status = spiInstance->transfer(command);
    digitalWrite(chipSelectPin, HIGH);
    return status;
}

void NRF24L01Driver::powerUp() {
    uint8_t config = readRegister(NRF24_CONFIG);
    writeRegister(NRF24_CONFIG, config | NRF24_PWR_UP);
    delayMicroseconds(150);
}

void NRF24L01Driver::powerDown() {
    uint8_t config = readRegister(NRF24_CONFIG);
    writeRegister(NRF24_CONFIG, config & ~NRF24_PWR_UP);
}

void NRF24L01Driver::setRxMode() {
    digitalWrite(chipEnablePin, LOW);
    uint8_t config = readRegister(NRF24_CONFIG);
    writeRegister(NRF24_CONFIG, config | NRF24_PRIM_RX);
    digitalWrite(chipEnablePin, HIGH);
    delayMicroseconds(130);
}

void NRF24L01Driver::setTxMode() {
    digitalWrite(chipEnablePin, LOW);
    uint8_t config = readRegister(NRF24_CONFIG);
    writeRegister(NRF24_CONFIG, config & ~NRF24_PRIM_RX);
}

void NRF24L01Driver::setChannel(uint8_t channel) {
    if (channel > 125) channel = 125;
    writeRegister(NRF24_RF_CH, channel);
    currentChannel = channel;
}

void NRF24L01Driver::setDataRate(uint8_t rate) {
    uint8_t setup = readRegister(NRF24_RF_SETUP);
    setup &= ~(NRF24_RF_DR_LOW | NRF24_RF_DR_HIGH);
    
    if (rate == 0) {
        setup |= NRF24_RF_DR_LOW; // 250 kbps
    } else if (rate == 2) {
        setup |= NRF24_RF_DR_HIGH; // 2 Mbps
    }
    // rate == 1 leaves both bits clear for 1 Mbps
    
    writeRegister(NRF24_RF_SETUP, setup);
}

void NRF24L01Driver::setPowerLevel(uint8_t level) {
    uint8_t setup = readRegister(NRF24_RF_SETUP);
    setup &= 0xF9; // Clear power bits
    setup |= ((level & 0x03) << 1);
    writeRegister(NRF24_RF_SETUP, setup);
}

void NRF24L01Driver::setAutoAck(bool enable) {
    writeRegister(NRF24_EN_AA, enable ? 0x3F : 0x00);
}

void NRF24L01Driver::setPayloadSize(uint8_t size) {
    if (size > 32) size = 32;
    writeRegister(NRF24_RX_PW_P0, size);
}

int8_t NRF24L01Driver::readRSSI() {
    // NRF24L01+ doesn't have true RSSI, use RPD (Received Power Detector)
    uint8_t rpd = getRPD();
    
    // Convert RPD to approximate dBm
    // RPD > 0 means signal > -64 dBm
    if (rpd > 0) {
        return -40; // Strong signal estimate
    } else {
        return -90; // Weak/no signal
    }
}

bool NRF24L01Driver::isCarrierDetected() {
    return getRPD() > 0;
}

uint8_t NRF24L01Driver::getRPD() {
    return readRegister(NRF24_RPD);
}

void NRF24L01Driver::transmitData(uint8_t* data, uint8_t length) {
    if (length > 32) length = 32;
    
    setTxMode();
    flushTx();
    
    digitalWrite(chipSelectPin, LOW);
    spiInstance->transfer(NRF24_W_TX_PAYLOAD);
    for (uint8_t i = 0; i < length; i++) {
        spiInstance->transfer(data[i]);
    }
    digitalWrite(chipSelectPin, HIGH);
    
    digitalWrite(chipEnablePin, HIGH);
    delayMicroseconds(15);
    digitalWrite(chipEnablePin, LOW);
    
    delay(1);
    setRxMode();
}

uint8_t NRF24L01Driver::receiveData(uint8_t* buffer, uint8_t maxLength) {
    uint8_t status = getStatus();
    
    if (!(status & 0x40)) { // RX_DR bit not set
        return 0;
    }
    
    uint8_t length = (maxLength < 32) ? maxLength : 32;
    
    digitalWrite(chipSelectPin, LOW);
    spiInstance->transfer(NRF24_R_RX_PAYLOAD);
    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = spiInstance->transfer(0xFF);
    }
    digitalWrite(chipSelectPin, HIGH);
    
    clearInterrupts();
    return length;
}

void NRF24L01Driver::flushRx() {
    sendCommand(NRF24_FLUSH_RX);
}

void NRF24L01Driver::flushTx() {
    sendCommand(NRF24_FLUSH_TX);
}

void NRF24L01Driver::clearInterrupts() {
    writeRegister(NRF24_STATUS, 0x70); // Clear all interrupt flags
}

uint8_t NRF24L01Driver::getStatus() {
    return sendCommand(NRF24_NOP);
}

bool NRF24L01Driver::isConnected() {
    uint8_t setup = readRegister(NRF24_SETUP_AW);
    return (setup >= 0x01 && setup <= 0x03);
}

void NRF24L01Driver::startListening() {
    setRxMode();
}

void NRF24L01Driver::stopListening() {
    digitalWrite(chipEnablePin, LOW);
}

// --- 2.4 GHz Spectrum Scanner ---

NRF24L01Driver::ChannelScanResult NRF24L01Driver::scanSpectrum(uint8_t passes, uint16_t dwellUs) {
    ChannelScanResult result;
    memset(result.activity, 0, sizeof(result.activity));
    result.peakChannel = 0;
    result.activeChannels = 0;
    
    // Save current state
    uint8_t savedChannel = currentChannel;
    
    // Disable auto-ack for raw scanning
    writeRegister(NRF24_EN_AA, 0x00);
    writeRegister(NRF24_RX_PW_P0, 32);
    
    for (uint8_t pass = 0; pass < passes; pass++) {
        for (uint8_t ch = 0; ch < 126; ch++) {
            // Set channel
            writeRegister(NRF24_RF_CH, ch);
            
            // Enter RX mode briefly
            setRxMode();
            delayMicroseconds(dwellUs);
            
            // Check RPD (Received Power Detector)
            // RPD is set if signal > -64 dBm was detected
            if (readRegister(NRF24_RPD) & 0x01) {
                if (result.activity[ch] < 255) {
                    result.activity[ch]++;
                }
            }
            
            stopListening();
        }
    }
    
    // Find peak and count active channels
    uint8_t maxActivity = 0;
    for (uint8_t ch = 0; ch < 126; ch++) {
        if (result.activity[ch] > 0) {
            result.activeChannels++;
        }
        if (result.activity[ch] > maxActivity) {
            maxActivity = result.activity[ch];
            result.peakChannel = ch;
        }
    }
    
    // Restore original channel
    setChannel(savedChannel);
    startListening();
    
    Serial.printf("[NRF24] Spectrum: %d active channels, peak ch%d (%d.%03d GHz)\n",
                  result.activeChannels, result.peakChannel,
                  2400 + result.peakChannel, 0);
    
    return result;
}

bool NRF24L01Driver::isChannelActive(uint8_t channel, uint8_t dwellUs) {
    if (channel > 125) return false;
    
    writeRegister(NRF24_RF_CH, channel);
    setRxMode();
    delayMicroseconds(dwellUs);
    bool active = (readRegister(NRF24_RPD) & 0x01);
    stopListening();
    
    // Restore
    setChannel(currentChannel);
    startListening();
    
    return active;
}

#endif // MODULE_NRF24
