#ifndef ACOUSTIC_DETECTOR_H
#define ACOUSTIC_DETECTOR_H

#include <Arduino.h>
#include <driver/i2s.h>

// I2S Configuration for MEMS microphone (ICS-43434 or similar)
#define I2S_PORT            I2S_NUM_0
#define I2S_SAMPLE_RATE     16000
#define I2S_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_32BIT
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
#define FREQ_RATIO_ON       0.008   // Alarm trigger threshold
#define FREQ_RATIO_OFF      0.004   // Alarm clear threshold
#define RMS_MIN             0.0003  // Minimum RMS to process
#define FREQS_NEEDED        1       // Min active frequencies
#define SUSTAIN_FRAMES_ON   2       // Frames to trigger
#define SUSTAIN_FRAMES_OFF  8       // Frames to clear

// Goertzel filter state
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
    AcousticDetector(uint8_t bclkPin, uint8_t wsPin, uint8_t dinPin);
    
    bool begin();
    void update();
    bool isDroneDetected() { return alarmState; }
    float getCurrentRatio() { return emaRatio; }
    
    void getFrequencyMagnitudes(float* magnitudes);
    void calibrate(uint16_t durationMs);
};

#endif // ACOUSTIC_DETECTOR_H
