#ifndef ACOUSTIC_DETECTOR_H
#define ACOUSTIC_DETECTOR_H

#include <Arduino.h>
#include "config.h"

#ifdef MODULE_ACOUSTIC

#include <driver/i2s.h>

// I2S Configuration
#define I2S_PORT            I2S_NUM_0
#define I2S_SAMPLE_RATE     16000
#define I2S_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT  // Fixed: was 32BIT with int16_t buffer
#define I2S_CHANNEL_FORMAT  I2S_CHANNEL_FMT_ONLY_LEFT

// Goertzel filter configuration
#define FRAME_SAMPLES       512
#define HOP_MS              100
#define NUM_TARGET_FREQS    6

// Drone rotor harmonic frequencies (Hz)
static const uint16_t DRONE_HARMONICS[NUM_TARGET_FREQS] = {
    200,   // Low frequency rotors
    400,   // Fundamental harmonic
    800,   // 2nd harmonic
    1200,  // 3rd harmonic
    2400,  // High-speed rotors
    4000   // Racing drones
};

// Detection thresholds
#define FREQ_RATIO_ON       0.008f
#define FREQ_RATIO_OFF      0.004f
#define RMS_MIN             0.0003f
#define FREQS_NEEDED        1
#define SUSTAIN_FRAMES_ON   2
#define SUSTAIN_FRAMES_OFF  8

struct GoertzelState {
    float coeff;
    float q1;
    float q2;
    float magnitude;
};

class AcousticDetector {
private:
    i2s_config_t i2sConfig;
    i2s_pin_config_t pinConfig;
    
    GoertzelState goertzelFilters[NUM_TARGET_FREQS];
    float emaRatio;
    uint8_t sustainCounter;
    bool alarmState;
    
    int16_t audioBuffer[FRAME_SAMPLES];
    
    void initializeGoertzel();
    void processGoertzelFrame(int16_t* samples, uint16_t length);
    float calculateRMS(int16_t* samples, uint16_t length);
    float calculateTonalRatio();
    
public:
    AcousticDetector(uint8_t bclkPin = PIN_I2S_BCLK, uint8_t wsPin = PIN_I2S_WS, uint8_t dinPin = PIN_I2S_DIN);
    
    bool begin();
    void update();
    bool isDroneDetected() { return alarmState; }
    float getCurrentRatio() { return emaRatio; }
    
    void getFrequencyMagnitudes(float* magnitudes);
    void calibrate(uint16_t durationMs);
};

#endif // MODULE_ACOUSTIC
#endif // ACOUSTIC_DETECTOR_H
