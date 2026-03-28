#include "rx5808.h"

RX5808Driver::RX5808Driver(uint8_t csPin, uint8_t rssiAnalogPin, SPIClass* spi) {
    chipSelectPin = csPin;
    rssiPin = rssiAnalogPin;
    spiInstance = spi;
    currentBand = RX5808_BAND_R; // Default to Raceband
    currentChannel = 0;
    initialized = false;
}

bool RX5808Driver::begin() {
    pinMode(chipSelectPin, OUTPUT);
    pinMode(rssiPin, INPUT);
    digitalWrite(chipSelectPin, HIGH);
    
    delay(100);
    
    // Set default channel (Raceband CH1 - 5658 MHz)
    setChannel(RX5808_BAND_R, 0);
    
    initialized = true;
    Serial.println("[RX5808] Initialized successfully");
    return true;
}

void RX5808Driver::writeRegister(uint32_t data) {
    digitalWrite(chipSelectPin, LOW);
    delayMicroseconds(1);
    
    // RX5808 uses 25-bit serial protocol
    // Send MSB first
    for (int8_t i = 24; i >= 0; i--) {
        digitalWrite(chipSelectPin, (data >> i) & 0x01);
        delayMicroseconds(1);
        
        // Clock pulse (using CS pin as clock in bit-bang mode)
        // Note: RX5808 has specific timing requirements
    }
    
    digitalWrite(chipSelectPin, HIGH);
    delayMicroseconds(1);
}

uint16_t RX5808Driver::getFrequencyForChannel(uint8_t band, uint8_t channel) {
    if (band > 4 || channel > 7) {
        return 5800; // Default fallback
    }
    return RX5808_FREQ_TABLE[band][channel];
}

uint32_t RX5808Driver::calculateSynthRegister(uint16_t frequency) {
    // RX5808 synthesizer calculation
    // Frequency = (N * 8 + A) * 2 + F
    // Where N is integer divider, A is fractional part
    
    uint16_t N = (frequency - 479) / 2;
    uint8_t A = (frequency - 479) % 2;
    
    // Build 25-bit register value
    // [24:20] = Address (0x01 for synth register)
    // [19:12] = N divider
    // [11:5]  = Reserved
    // [4]     = A bit
    // [3:0]   = Control bits
    
    uint32_t synthReg = 0x01000000; // Address 0x01
    synthReg |= ((uint32_t)N << 7);
    synthReg |= ((uint32_t)A << 4);
    synthReg |= 0x08; // Control bits
    
    return synthReg;
}

void RX5808Driver::setChannel(uint8_t band, uint8_t channel) {
    if (band > 4 || channel > 7) {
        Serial.println("[RX5808] Invalid band/channel");
        return;
    }
    
    uint16_t frequency = getFrequencyForChannel(band, channel);
    setFrequency(frequency);
    
    currentBand = band;
    currentChannel = channel;
    
    Serial.printf("[RX5808] Set to %s CH%d (%d MHz)\n", 
                  getBandName(band), channel + 1, frequency);
}

void RX5808Driver::setFrequency(uint16_t frequencyMHz) {
    uint32_t synthReg = calculateSynthRegister(frequencyMHz);
    writeRegister(synthReg);
    delay(30); // Allow PLL to lock
}

int RX5808Driver::readRSSI() {
    int rawValue = readRSSIRaw();
    // Convert 12-bit ADC (0-4095) to percentage (0-100)
    return map(rawValue, 0, 4095, 0, 100);
}

int RX5808Driver::readRSSIRaw() {
    // Average multiple readings for stability
    uint32_t sum = 0;
    const uint8_t samples = 10;
    
    for (uint8_t i = 0; i < samples; i++) {
        sum += analogRead(rssiPin);
        delayMicroseconds(100);
    }
    
    return sum / samples;
}

void RX5808Driver::scanBand(uint8_t band, int* rssiValues) {
    if (band > 4) {
        Serial.println("[RX5808] Invalid band");
        return;
    }
    
    Serial.printf("[RX5808] Scanning %s band...\n", getBandName(band));
    
    for (uint8_t ch = 0; ch < 8; ch++) {
        setChannel(band, ch);
        delay(50); // Settling time
        rssiValues[ch] = readRSSI();
        Serial.printf("  CH%d (%d MHz): %d%%\n", 
                     ch + 1, 
                     getFrequencyForChannel(band, ch), 
                     rssiValues[ch]);
    }
}

RX5808Channel RX5808Driver::findStrongestChannel() {
    RX5808Channel strongest = {0, 0, 0};
    int maxRSSI = 0;
    
    Serial.println("[RX5808] Scanning all bands for strongest signal...");
    
    for (uint8_t band = 0; band < 5; band++) {
        for (uint8_t ch = 0; ch < 8; ch++) {
            setChannel(band, ch);
            delay(50);
            int rssi = readRSSI();
            
            if (rssi > maxRSSI) {
                maxRSSI = rssi;
                strongest.band = band;
                strongest.channel = ch;
                strongest.frequency = getFrequencyForChannel(band, ch);
            }
        }
    }
    
    Serial.printf("[RX5808] Strongest: %s CH%d (%d MHz) - %d%%\n",
                  getBandName(strongest.band),
                  strongest.channel + 1,
                  strongest.frequency,
                  maxRSSI);
    
    return strongest;
}

uint16_t RX5808Driver::getCurrentFrequency() {
    return getFrequencyForChannel(currentBand, currentChannel);
}

const char* RX5808Driver::getBandName(uint8_t band) {
    switch(band) {
        case RX5808_BAND_A: return "Boscam A";
        case RX5808_BAND_B: return "Boscam B";
        case RX5808_BAND_E: return "Boscam E";
        case RX5808_BAND_F: return "Fatshark";
        case RX5808_BAND_R: return "Raceband";
        default: return "Unknown";
    }
}
