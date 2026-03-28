#ifndef ML_CLASSIFIER_H
#define ML_CLASSIFIER_H

#include <Arduino.h>
#include "config.h"

#ifdef MODULE_ML

enum DroneClassification {
    CLASS_UNKNOWN = 0,
    CLASS_DJI_PHANTOM = 1,
    CLASS_DJI_MAVIC = 2,
    CLASS_FPV_RACING = 3,
    CLASS_MILITARY = 4
};

struct ClassificationResult {
    DroneClassification droneClass;
    float confidence;
    uint32_t inferenceTimeMs;
    bool isValid;
};

struct RFFeatures {
    float rssiHistory[RSSI_HISTORY_SIZE];
    float frequencySpectrum[64];
    float modulationIndex;
    float signalBandwidth;
    float peakPower;
    float avgPower;
    uint8_t burstPattern[32];
    
    // Protocol detection flags (from MAVLink/CRSF parsers)
    bool mavlinkDetected;
    bool crsfDetected;
    bool djiPatternDetected;
    bool analogVideoDetected;
    
    // Band activity
    bool band900Active;
    bool band2400Active;
    bool band5800Active;
};

class MLClassifier {
private:
    bool modelLoaded;
    float inputBuffer[ML_INPUT_SIZE];
    float outputBuffer[ML_OUTPUT_SIZE];
    
    // Rule-based classification
    ClassificationResult classifyRuleBased(const RFFeatures& features);
    
    // Helper
    DroneClassification argmax(const float* output, size_t length);

public:
    MLClassifier();
    
    bool begin();
    bool loadModel(const uint8_t* modelData, size_t modelSize);
    
    ClassificationResult classify(const RFFeatures& features);
    ClassificationResult classifyFromRSSI(const int* rssiValues, size_t count,
                                          bool mavlink = false, bool crsf = false,
                                          bool band900 = false, bool band2400 = false, bool band5800 = false);
    
    const char* getClassName(DroneClassification droneClass);
    bool isModelLoaded() const;
    
    void printClassificationResult(const ClassificationResult& result);
};

#endif // MODULE_ML
#endif // ML_CLASSIFIER_H
