#ifndef ML_CLASSIFIER_H
#define ML_CLASSIFIER_H

#include <Arduino.h>

#define MODEL_INPUT_SIZE 128
#define MODEL_OUTPUT_SIZE 5
#define INFERENCE_THRESHOLD 0.7

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
    float rssiHistory[32];
    float frequencySpectrum[64];
    float modulationIndex;
    float signalBandwidth;
    float peakPower;
    float avgPower;
    uint8_t burstPattern[32];
};

class MLClassifier {
private:
    bool modelLoaded;
    float inputBuffer[MODEL_INPUT_SIZE];
    float outputBuffer[MODEL_OUTPUT_SIZE];
    
    void preprocessFeatures(const RFFeatures& features);
    DroneClassification argmax(const float* output, size_t length);
    float softmax(const float* input, size_t index, size_t length);

public:
    MLClassifier();
    
    bool begin();
    bool loadModel(const uint8_t* modelData, size_t modelSize);
    
    ClassificationResult classify(const RFFeatures& features);
    ClassificationResult classifyFromRSSI(const int* rssiValues, size_t count);
    
    const char* getClassName(DroneClassification droneClass);
    bool isModelLoaded() const;
    
    void printClassificationResult(const ClassificationResult& result);
};

#endif
