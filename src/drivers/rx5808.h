#ifndef RX5808_H
#define RX5808_H

#include <Arduino.h>
#include <SPI.h>
#include "../config.h"

#ifdef MODULE_RX5808

// RX5808 Frequency Bands (5.8 GHz)
#define RX5808_BAND_A   0
#define RX5808_BAND_B   1
#define RX5808_BAND_E   2
#define RX5808_BAND_F   3
#define RX5808_BAND_R   4

// Channel definitions per band (8 channels each)
struct RX5808Channel {
    uint8_t band;
    uint8_t channel;
    uint16_t frequency; // MHz
};

class RX5808Driver {
private:
    uint8_t chipSelectPin;
    uint8_t rssiPin;
    SPIClass* spiInstance;
    uint8_t currentBand;
    uint8_t currentChannel;
    bool initialized;
    
    void writeRegister(uint32_t data);
    uint16_t getFrequencyForChannel(uint8_t band, uint8_t channel);
    uint32_t calculateSynthRegister(uint16_t frequency);
    
public:
    RX5808Driver(uint8_t csPin, uint8_t rssiAnalogPin, SPIClass* spi = &SPI);
    
    bool begin();
    void setChannel(uint8_t band, uint8_t channel);
    void setFrequency(uint16_t frequencyMHz);
    
    int readRSSI(); // Returns 0-100 percentage
    int readRSSIRaw(); // Returns raw ADC value (0-4095)
    
    void scanBand(uint8_t band, int* rssiValues);
    RX5808Channel findStrongestChannel();
    
    uint16_t getCurrentFrequency();
    const char* getBandName(uint8_t band);
};

// Frequency table for all bands
static const uint16_t RX5808_FREQ_TABLE[5][8] = {
    // Band A (Boscam A)
    {5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725},
    // Band B (Boscam B)
    {5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866},
    // Band E (Boscam E / DJI)
    {5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945},
    // Band F (Fatshark / IRC)
    {5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880},
    // Band R (Raceband)
    {5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917}
};

#endif // MODULE_RX5808
#endif // RX5808_H
