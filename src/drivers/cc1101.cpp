#include "cc1101.h"

CC1101Driver::CC1101Driver(uint8_t csPin, SPIClass* spi) {
    chipSelectPin = csPin;
    spiInstance = spi;
    currentFrequency = 915000000; // Default 915 MHz
    initialized = false;
}

bool CC1101Driver::begin() {
    pinMode(chipSelectPin, OUTPUT);
    digitalWrite(chipSelectPin, HIGH);
    
    delay(50);
    reset();
    delay(50);
    
    // Verify chip connection
    uint8_t version = getChipVersion();
    if (version == 0x00 || version == 0xFF) {
        Serial.println("[CC1101] Chip not detected");
        return false;
    }
    
    Serial.printf("[CC1101] Chip version: 0x%02X\n", version);
    
    // Configure for 915 MHz ISM band, GFSK modulation
    writeRegister(CC1101_IOCFG2, 0x29);    // GDO2 output pin config
    writeRegister(CC1101_IOCFG0, 0x06);    // GDO0 output pin config
    writeRegister(CC1101_FIFOTHR, 0x47);   // FIFO threshold
    writeRegister(CC1101_PKTCTRL0, 0x05);  // Packet automation control
    writeRegister(CC1101_FSCTRL1, 0x06);   // Frequency synthesizer control
    
    setFrequency(915000000);
    setModulation(CC1101_MOD_GFSK);
    setDataRate(250000); // 250 kbps
    
    writeRegister(CC1101_MDMCFG1, 0x22);   // Modem configuration
    writeRegister(CC1101_MDMCFG0, 0xF8);   // Modem configuration
    writeRegister(CC1101_DEVIATN, 0x47);   // Modem deviation
    writeRegister(CC1101_MCSM0, 0x18);     // Main Radio Control State Machine
    writeRegister(CC1101_FOCCFG, 0x16);    // Frequency Offset Compensation
    writeRegister(CC1101_AGCCTRL2, 0x43);  // AGC control
    writeRegister(CC1101_AGCCTRL1, 0x40);
    writeRegister(CC1101_AGCCTRL0, 0x91);
    writeRegister(CC1101_FREND1, 0x56);    // Front end RX configuration
    writeRegister(CC1101_FREND0, 0x10);    // Front end TX configuration
    writeRegister(CC1101_FSCAL3, 0xE9);    // Frequency synthesizer calibration
    writeRegister(CC1101_FSCAL2, 0x2A);
    writeRegister(CC1101_FSCAL1, 0x00);
    writeRegister(CC1101_FSCAL0, 0x1F);
    
    setPowerLevel(0xC0); // Max power
    
    initialized = true;
    setRxMode();
    
    Serial.println("[CC1101] Initialized successfully");
    return true;
}

void CC1101Driver::reset() {
    digitalWrite(chipSelectPin, LOW);
    delayMicroseconds(10);
    digitalWrite(chipSelectPin, HIGH);
    delayMicroseconds(40);
    
    sendStrobe(CC1101_SRES);
    delay(10);
}

void CC1101Driver::writeRegister(uint8_t address, uint8_t value) {
    digitalWrite(chipSelectPin, LOW);
    while(digitalRead(19)); // Wait for MISO to go low
    spiInstance->transfer(address);
    spiInstance->transfer(value);
    digitalWrite(chipSelectPin, HIGH);
}

uint8_t CC1101Driver::readRegister(uint8_t address) {
    digitalWrite(chipSelectPin, LOW);
    spiInstance->transfer(address | 0x80); // Read bit
    uint8_t value = spiInstance->transfer(0x00);
    digitalWrite(chipSelectPin, HIGH);
    return value;
}

void CC1101Driver::sendStrobe(uint8_t strobe) {
    digitalWrite(chipSelectPin, LOW);
    spiInstance->transfer(strobe);
    digitalWrite(chipSelectPin, HIGH);
}

uint8_t CC1101Driver::readStatusRegister(uint8_t address) {
    digitalWrite(chipSelectPin, LOW);
    spiInstance->transfer(address | 0xC0); // Burst read bit
    uint8_t value = spiInstance->transfer(0x00);
    digitalWrite(chipSelectPin, HIGH);
    return value;
}

void CC1101Driver::setFrequency(uint32_t frequencyHz) {
    // Calculate frequency registers
    // FREQ = (Desired Frequency * 2^16) / 26 MHz
    uint32_t freq = (frequencyHz * 65536ULL) / 26000000ULL;
    
    writeRegister(CC1101_FREQ2, (freq >> 16) & 0xFF);
    writeRegister(CC1101_FREQ1, (freq >> 8) & 0xFF);
    writeRegister(CC1101_FREQ0, freq & 0xFF);
    
    currentFrequency = frequencyHz;
    Serial.printf("[CC1101] Frequency set to %u Hz\n", frequencyHz);
}

void CC1101Driver::setModulation(uint8_t modulation) {
    uint8_t mdmcfg2 = readRegister(CC1101_MDMCFG2);
    mdmcfg2 = (mdmcfg2 & 0x8F) | modulation;
    writeRegister(CC1101_MDMCFG2, mdmcfg2);
}

void CC1101Driver::setDataRate(uint32_t baud) {
    // Calculate DRATE_M and DRATE_E
    // Data Rate = (256 + DRATE_M) * 2^DRATE_E * (26MHz / 2^28)
    uint8_t drate_e = 0;
    uint32_t drate_m = 0;
    
    // Simplified calculation for common rates
    if (baud >= 250000) {
        drate_e = 13;
        drate_m = 59;
    } else if (baud >= 115200) {
        drate_e = 12;
        drate_m = 131;
    } else {
        drate_e = 11;
        drate_m = 248;
    }
    
    writeRegister(CC1101_MDMCFG4, (0xF << 4) | drate_e);
    writeRegister(CC1101_MDMCFG3, drate_m);
}

void CC1101Driver::setChannel(uint8_t channel) {
    writeRegister(CC1101_CHANNR, channel);
}

void CC1101Driver::setPowerLevel(uint8_t power) {
    writeRegister(CC1101_PATABLE, power);
}

void CC1101Driver::setRxMode() {
    sendStrobe(CC1101_SRX);
}

void CC1101Driver::setTxMode() {
    sendStrobe(CC1101_STX);
}

void CC1101Driver::setIdleMode() {
    sendStrobe(CC1101_SIDLE);
}

int8_t CC1101Driver::readRSSI() {
    uint8_t rssiRaw = readStatusRegister(CC1101_RSSI);
    
    // Convert to dBm
    int16_t rssiDbm;
    if (rssiRaw >= 128) {
        rssiDbm = (rssiRaw - 256) / 2 - 74;
    } else {
        rssiDbm = rssiRaw / 2 - 74;
    }
    
    return (int8_t)rssiDbm;
}

uint8_t CC1101Driver::getLQI() {
    return readStatusRegister(CC1101_LQI) & 0x7F;
}

bool CC1101Driver::isCarrierDetected() {
    uint8_t pktstatus = readStatusRegister(CC1101_PKTSTATUS);
    return (pktstatus & 0x10) != 0; // CCA bit
}

void CC1101Driver::transmitData(uint8_t* data, uint8_t length) {
    setIdleMode();
    flushTxFIFO();
    
    digitalWrite(chipSelectPin, LOW);
    spiInstance->transfer(CC1101_TXFIFO | 0x40); // Burst write
    spiInstance->transfer(length);
    for (uint8_t i = 0; i < length; i++) {
        spiInstance->transfer(data[i]);
    }
    digitalWrite(chipSelectPin, HIGH);
    
    setTxMode();
    
    // Wait for transmission to complete
    delay((length * 8 * 1000) / 250000 + 5);
    setRxMode();
}

uint8_t CC1101Driver::receiveData(uint8_t* buffer, uint8_t maxLength) {
    uint8_t rxBytes = readStatusRegister(CC1101_RXBYTES);
    
    if (rxBytes & 0x80) {
        flushRxFIFO();
        return 0;
    }
    
    uint8_t numBytes = rxBytes & 0x7F;
    if (numBytes == 0 || numBytes > maxLength) {
        return 0;
    }
    
    digitalWrite(chipSelectPin, LOW);
    spiInstance->transfer(CC1101_RXFIFO | 0xC0); // Burst read
    uint8_t length = spiInstance->transfer(0x00);
    
    for (uint8_t i = 0; i < length && i < maxLength; i++) {
        buffer[i] = spiInstance->transfer(0x00);
    }
    digitalWrite(chipSelectPin, HIGH);
    
    return length;
}

void CC1101Driver::flushRxFIFO() {
    sendStrobe(CC1101_SFRX);
}

void CC1101Driver::flushTxFIFO() {
    sendStrobe(CC1101_SFTX);
}

uint8_t CC1101Driver::getChipVersion() {
    return readStatusRegister(CC1101_VERSION);
}

bool CC1101Driver::isConnected() {
    uint8_t version = getChipVersion();
    return (version != 0x00 && version != 0xFF);
}
