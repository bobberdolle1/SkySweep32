#include "acoustic_detector.h"
#include <math.h>

#define EMA_ALPHA 0.25

AcousticDetector::AcousticDetector(uint8_t bclkPin, uint8_t wsPin, uint8_t dinPin) {
    emaRatio = 0.0f;
    sustainCounter = 0;
    alarmState = false;
    
    // I2S configuration
    i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FORMAT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    pinConfig = {
        .bck_io_num = bclkPin,
        .ws_io_num = wsPin,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = dinPin
    };
}

bool AcousticDetector::begin() {
    esp_err_t err = i2s_driver_install(I2S_PORT, &i2sConfig, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[ACOUSTIC] I2S driver install failed: %d\n", err);
        return false;
    }
    
    err = i2s_set_pin(I2S_PORT, &pinConfig);
    if (err != ESP_OK) {
        Serial.printf("[ACOUSTIC] I2S pin config failed: %d\n", err);
        return false;
    }
    
    initializeGoertzel();
    
    Serial.println("[ACOUSTIC] Initialized successfully");
    Serial.println("[ACOUSTIC] Target frequencies:");
    for (uint8_t i = 0; i < NUM_TARGET_FREQS; i++) {
        Serial.printf("  [%d] %d Hz\n", i, DRONE_HARMONICS[i]);
    }
    
    return true;
}

void AcousticDetector::initializeGoertzel() {
    for (uint8_t i = 0; i < NUM_TARGET_FREQS; i++) {
        float omega = (2.0f * PI * DRONE_HARMONICS[i]) / I2S_SAMPLE_RATE;
        goertzelFilters[i].coeff = 2.0f * cosf(omega);
        goertzelFilters[i].q1 = 0.0f;
        goertzelFilters[i].q2 = 0.0f;
        goertzelFilters[i].magnitude = 0.0f;
    }
}

void AcousticDetector::update() {
    size_t bytesRead = 0;
    
    // Read audio samples from I2S
    i2s_read(I2S_PORT, audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);
    
    if (bytesRead == 0) {
        return;
    }
    
    uint16_t samplesRead = bytesRead / sizeof(int16_t);
    
    // Calculate RMS to filter out silence
    float rms = calculateRMS(audioBuffer, samplesRead);
    if (rms < RMS_MIN) {
        return; // Skip silent frames
    }
    
    // Process Goertzel filters
    processGoertzelFrame(audioBuffer, samplesRead);
    
    // Calculate tonal/broadband ratio
    float ratio = calculateTonalRatio();
    
    // Apply EMA smoothing
    emaRatio = EMA_ALPHA * ratio + (1.0f - EMA_ALPHA) * emaRatio;
    
    // Sustain logic for alarm state
    if (emaRatio > FREQ_RATIO_ON) {
        sustainCounter++;
        if (sustainCounter >= SUSTAIN_FRAMES_ON && !alarmState) {
            alarmState = true;
            Serial.println("[ACOUSTIC] *** DRONE DETECTED (ACOUSTIC) ***");
        }
    } else if (emaRatio < FREQ_RATIO_OFF) {
        if (sustainCounter > 0) sustainCounter--;
        if (sustainCounter == 0 && alarmState) {
            alarmState = false;
            Serial.println("[ACOUSTIC] Drone signal lost");
        }
    }
}

void AcousticDetector::processGoertzelFrame(int16_t* samples, uint16_t length) {
    // Reset Goertzel states
    for (uint8_t i = 0; i < NUM_TARGET_FREQS; i++) {
        goertzelFilters[i].q1 = 0.0f;
        goertzelFilters[i].q2 = 0.0f;
    }
    
    // Process each sample through all Goertzel filters
    for (uint16_t n = 0; n < length; n++) {
        float sample = (float)samples[n] / 32768.0f; // Normalize to [-1, 1]
        
        for (uint8_t i = 0; i < NUM_TARGET_FREQS; i++) {
            float q0 = goertzelFilters[i].coeff * goertzelFilters[i].q1 
                      - goertzelFilters[i].q2 + sample;
            goertzelFilters[i].q2 = goertzelFilters[i].q1;
            goertzelFilters[i].q1 = q0;
        }
    }
    
    // Calculate magnitudes
    for (uint8_t i = 0; i < NUM_TARGET_FREQS; i++) {
        float real = goertzelFilters[i].q1 - goertzelFilters[i].q2 * cosf(
            (2.0f * PI * DRONE_HARMONICS[i]) / I2S_SAMPLE_RATE);
        float imag = goertzelFilters[i].q2 * sinf(
            (2.0f * PI * DRONE_HARMONICS[i]) / I2S_SAMPLE_RATE);
        goertzelFilters[i].magnitude = sqrtf(real * real + imag * imag) / length;
    }
}

float AcousticDetector::calculateRMS(int16_t* samples, uint16_t length) {
    float sum = 0.0f;
    for (uint16_t i = 0; i < length; i++) {
        float sample = (float)samples[i] / 32768.0f;
        sum += sample * sample;
    }
    return sqrtf(sum / length);
}

float AcousticDetector::calculateTonalRatio() {
    float tonalEnergy = 0.0f;
    uint8_t activeFreqs = 0;
    
    for (uint8_t i = 0; i < NUM_TARGET_FREQS; i++) {
        tonalEnergy += goertzelFilters[i].magnitude;
        if (goertzelFilters[i].magnitude > 0.001f) {
            activeFreqs++;
        }
    }
    
    if (activeFreqs < FREQS_NEEDED) {
        return 0.0f;
    }
    
    // Broadband energy approximation (total RMS)
    float broadbandEnergy = calculateRMS(audioBuffer, FRAME_SAMPLES);
    
    if (broadbandEnergy < 0.0001f) {
        return 0.0f;
    }
    
    return tonalEnergy / broadbandEnergy;
}

void AcousticDetector::getFrequencyMagnitudes(float* magnitudes) {
    for (uint8_t i = 0; i < NUM_TARGET_FREQS; i++) {
        magnitudes[i] = goertzelFilters[i].magnitude;
    }
}

void AcousticDetector::calibrate(uint16_t durationMs) {
    Serial.println("[ACOUSTIC] Starting calibration...");
    Serial.println("[ACOUSTIC] Play drone audio or fly a drone nearby");
    
    uint32_t startTime = millis();
    float maxRatio = 0.0f;
    float minRatio = 1.0f;
    
    while (millis() - startTime < durationMs) {
        update();
        
        if (emaRatio > maxRatio) maxRatio = emaRatio;
        if (emaRatio < minRatio && emaRatio > 0.0001f) minRatio = emaRatio;
        
        Serial.printf("[CAL] Ratio: %.6f | Active: %d/%d\n", 
                     emaRatio, sustainCounter, SUSTAIN_FRAMES_ON);
        delay(HOP_MS);
    }
    
    Serial.printf("[ACOUSTIC] Calibration complete\n");
    Serial.printf("[ACOUSTIC] Max ratio: %.6f\n", maxRatio);
    Serial.printf("[ACOUSTIC] Min ratio: %.6f\n", minRatio);
    Serial.printf("[ACOUSTIC] Suggested FREQ_RATIO_ON: %.6f\n", maxRatio * 0.7f);
    Serial.printf("[ACOUSTIC] Suggested FREQ_RATIO_OFF: %.6f\n", minRatio * 1.5f);
}
