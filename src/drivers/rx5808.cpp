#include "rx5808.h"

#ifdef MODULE_RX5808

namespace {

constexpr uint16_t kBandEFrequenciesMHz[8] = {
    5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945,
};
constexpr int kRssiMinimumMilliVolts = 350;
constexpr int kRssiMaximumMilliVolts = 1400;
constexpr uint8_t kRssiSamples = 8;

}  // namespace

RX5808Driver::RX5808Driver(uint8_t channel1Pin, uint8_t channel2Pin,
                           uint8_t channel3Pin, uint8_t rssiAnalogPin)
    : channel1Pin(channel1Pin),
      channel2Pin(channel2Pin),
      channel3Pin(channel3Pin),
      rssiPin(rssiAnalogPin),
      currentChannel(0),
      initialized(false),
      scanStarted(false) {}

bool RX5808Driver::begin() {
    if (channel1Pin == channel2Pin || channel1Pin == channel3Pin ||
        channel2Pin == channel3Pin) {
        Serial.println("[RX5808] ERROR: three independent channel-select pins required");
        return false;
    }

    pinMode(channel1Pin, OUTPUT);
    pinMode(channel2Pin, OUTPUT);
    pinMode(channel3Pin, OUTPUT);
    pinMode(rssiPin, INPUT);
    setChannel(0);
    initialized = true;
    Serial.println("[RX5808] Initialized with Rev C three-bit channel selection");
    return true;
}

uint16_t RX5808Driver::frequencyForChannel(uint8_t channel) const {
    return channel < 8 ? kBandEFrequenciesMHz[channel] : 0;
}

void RX5808Driver::setChannel(uint8_t channel) {
    if (channel >= 8) {
        Serial.println("[RX5808] Invalid Rev C channel");
        return;
    }
    digitalWrite(channel1Pin, (channel & 0b001) ? HIGH : LOW);
    digitalWrite(channel2Pin, (channel & 0b010) ? HIGH : LOW);
    digitalWrite(channel3Pin, (channel & 0b100) ? HIGH : LOW);
    delay(30);
    currentChannel = channel;
    Serial.printf("[RX5808] Band E CH%d (%d MHz)\n", channel + 1,
                  frequencyForChannel(channel));
}

void RX5808Driver::setFrequency(uint16_t frequencyMHz) {
    for (uint8_t channel = 0; channel < 8; ++channel) {
        if (frequencyForChannel(channel) == frequencyMHz) {
            setChannel(channel);
            return;
        }
    }
    Serial.println("[RX5808] Frequency unavailable with Rev C strap control");
}

int RX5808Driver::readRSSI() {
    // Conservative uncalibrated endpoints for common RX5808 RSSI outputs.
    // Prototype measurements must precede field-strength or range claims.
    uint32_t sumMilliVolts = 0;
    for (uint8_t sample = 0; sample < kRssiSamples; ++sample) {
        sumMilliVolts += analogReadMilliVolts(rssiPin);
        delayMicroseconds(100);
    }
    const int averageMilliVolts = sumMilliVolts / kRssiSamples;
    return constrain(map(averageMilliVolts, kRssiMinimumMilliVolts,
                         kRssiMaximumMilliVolts, 0, 100),
                     0, 100);
}

int RX5808Driver::scanNextChannel() {
    if (!initialized) return 0;
    if (scanStarted) {
        setChannel((currentChannel + 1) % 8);
    } else {
        scanStarted = true;
    }
    return readRSSI();
}

int RX5808Driver::readRSSIRaw() {
    uint32_t sum = 0;
    constexpr uint8_t kSamples = 10;
    for (uint8_t sample = 0; sample < kSamples; ++sample) {
        sum += analogRead(rssiPin);
        delayMicroseconds(100);
    }
    return sum / kSamples;
}

void RX5808Driver::scanBand(int* rssiValues) {
    if (rssiValues == nullptr) return;
    for (uint8_t channel = 0; channel < 8; ++channel) {
        setChannel(channel);
        rssiValues[channel] = readRSSI();
    }
}

RX5808Channel RX5808Driver::findStrongestChannel() {
    RX5808Channel strongest = {RX5808_BAND_E, 0, frequencyForChannel(0)};
    int strongestRssi = -1;
    for (uint8_t channel = 0; channel < 8; ++channel) {
        setChannel(channel);
        const int rssi = readRSSI();
        if (rssi > strongestRssi) {
            strongestRssi = rssi;
            strongest = {RX5808_BAND_E, channel, frequencyForChannel(channel)};
        }
    }
    Serial.printf("[RX5808] Strongest Band E CH%d (%d MHz): %d%%\n",
                  strongest.channel + 1, strongest.frequency, strongestRssi);
    return strongest;
}

uint16_t RX5808Driver::getCurrentFrequency() const {
    return frequencyForChannel(currentChannel);
}

#endif  // MODULE_RX5808
