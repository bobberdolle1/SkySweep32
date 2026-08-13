#include "cc1101.h"

#ifdef MODULE_CC1101

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
    // Wait for MISO (chip-ready) to go low, but never hang forever if the chip
    // is absent or faulty (previously an unbounded busy-wait on a hardcoded pin).
    uint32_t t0 = millis();
    while (digitalRead(PIN_SPI_MISO)) {
        if (millis() - t0 > 10) break;
    }
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
    if (frequencyHz < 855000000 || frequencyHz > 925000000) {
        Serial.printf("[CC1101] Refusing out-of-contract frequency %u Hz\n", frequencyHz);
        return;
    }

    // FREQ = desired frequency * 2^16 / 26 MHz.
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


void CC1101Driver::setRxMode() {
    sendStrobe(CC1101_SRX);
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

    // Return only what was actually copied — the on-air length byte is
    // attacker-controlled and returning it raw makes the caller read past its buffer.
    return (length < maxLength) ? length : maxLength;
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

// --- Rev C sub-GHz receive contract ---

static const uint32_t BAND_FREQUENCIES[] = {
    860000000,
    890000000,
    920000000,
};

static const char* BAND_NAMES[] = {"860MHz", "890MHz", "920MHz"};

void CC1101Driver::setBand(FreqBand band) {
    if (band >= BAND_COUNT) return;
    
    setIdleMode();
    setFrequency(BAND_FREQUENCIES[band]);
    
    // Recalibrate after frequency change
    sendStrobe(CC1101_SCAL);
    delay(1);
    
    setRxMode();
    Serial.printf("[CC1101] Band: %s\n", BAND_NAMES[band]);
}

CC1101Driver::BandScanResult CC1101Driver::scanAllBands(uint16_t dwellMs) {
    BandScanResult result;
    uint32_t saved = currentFrequency;  // setFrequency() in the loop overwrites currentFrequency

    for (uint8_t b = 0; b < BAND_COUNT; b++) {
        setIdleMode();
        setFrequency(BAND_FREQUENCIES[b]);
        sendStrobe(CC1101_SCAL);
        delayMicroseconds(800);
        setRxMode();
        delay(dwellMs);
        
        result.rssi[b] = readRSSI();
        result.activity[b] = (result.rssi[b] > -75);  // Activity threshold
    }
    
    // Restore the pre-scan frequency (the loop's setFrequency calls overwrote currentFrequency)
    setFrequency(saved);
    sendStrobe(CC1101_SCAL);
    delayMicroseconds(800);
    setRxMode();
    
    return result;
}

void CC1101Driver::spectrumScan(uint32_t startHz, uint32_t endHz, uint32_t stepHz,
                                 int8_t* rssiOut, uint16_t maxPoints, uint16_t dwellMs) {
    if (startHz < 855000000 || endHz > 925000000 || startHz > endHz || stepHz == 0) {
        Serial.println("[CC1101] Refusing out-of-contract spectrum scan");
        return;
    }
    uint16_t idx = 0;
    uint32_t saved = currentFrequency;  // setFrequency() in the loop overwrites currentFrequency
    
    for (uint32_t freq = startHz; freq <= endHz && idx < maxPoints; freq += stepHz, idx++) {
        setIdleMode();
        setFrequency(freq);
        sendStrobe(CC1101_SCAL);
        delayMicroseconds(800);
        setRxMode();
        delay(dwellMs);
        
        rssiOut[idx] = readRSSI();
    }
    
    // Restore original frequency
    setFrequency(saved);
    sendStrobe(CC1101_SCAL);
    delayMicroseconds(800);
    setRxMode();
    
    Serial.printf("[CC1101] Spectrum scan: %u-%u MHz, %u points\n",
                  startHz / 1000000, endHz / 1000000, idx);
}

#endif // MODULE_CC1101
