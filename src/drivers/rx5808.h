#ifndef RX5808_H
#define RX5808_H

#include <Arduino.h>
#include "../config.h"

#ifdef MODULE_RX5808

// Rev C fits the documented 12-pad RX5808 module in three-bit strap mode.
// It can observe only its eight hardware-selected Band-E channels.
#define RX5808_BAND_E 2

struct RX5808Channel {
    uint8_t band;
    uint8_t channel;
    uint16_t frequency;  // MHz
};

class RX5808Driver {
private:
    uint8_t channel1Pin;
    uint8_t channel2Pin;
    uint8_t channel3Pin;
    uint8_t rssiPin;
    uint8_t currentChannel;
    bool initialized;
    bool scanStarted;

    uint16_t frequencyForChannel(uint8_t channel) const;

public:
    RX5808Driver(uint8_t channel1Pin, uint8_t channel2Pin, uint8_t channel3Pin,
                 uint8_t rssiAnalogPin);

    bool begin();
    void setChannel(uint8_t channel);
    void setFrequency(uint16_t frequencyMHz);

    int readRSSI();       // Relative 0–100 display value; not calibrated power.
    int scanNextChannel();
    int readRSSIRaw();

    void scanBand(int* rssiValues);
    RX5808Channel findStrongestChannel();
    uint16_t getCurrentFrequency() const;
};

#endif  // MODULE_RX5808
#endif  // RX5808_H
