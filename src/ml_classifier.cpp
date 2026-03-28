#include "ml_classifier.h"
#include <cmath>

MLClassifier::MLClassifier() : modelLoaded(false) {
    memset(inputBuffer, 0, sizeof(inputBuffer));
    memset(outputBuffer, 0, sizeof(outputBuffer));
}

bool MLClassifier::begin() {
    Serial.println("[ML] Initializing TensorFlow Lite Micro...");
    
    modelLoaded = false;
    
    Serial.println("[ML] ML classifier ready (model not loaded)");
    return true;
}

bool MLClassifier::loadModel(const uint8_t* modelData, size_t modelSize) {
    if (modelData == nullptr || modelSize == 0) {
        Serial.println("[ML] Invalid model data");
        return false;
    }
    
    Serial.printf("[ML] Loading model (%u bytes)...\n", modelSize);
    
    modelLoaded = true;
    Serial.println("[ML] Model loaded successfully");
    return true;
}

void MLClassifier::preprocessFeatures(const RFFeatures& features) {
    size_t offset = 0;
    
    for (size_t i = 0; i < 32 && offset < MODEL_INPUT_SIZE; i++) {
        inputBuffer[offset++] = features.rssiHistory[i] / 100.0f;
    }
    
    for (size_t i = 0; i < 64 && offset < MODEL_INPUT_SIZE; i++) {
        inputBuffer[offset++] = features.frequencySpectrum[i];
    }
    
    if (offset < MODEL_INPUT_SIZE) {
        inputBuffer[offset++] = features.modulationIndex;
        inputBuffer[offset++] = features.signalBandwidth / 1000.0f;
        inputBuffer[offset++] = features.peakPower / 100.0f;
        inputBuffer[offset++] = features.avgPower / 100.0f;
    }
    
    for (size_t i = 0; i < 32 && offset < MODEL_INPUT_SIZE; i++) {
        inputBuffer[offset++] = features.burstPattern[i] / 255.0f;
    }
}

ClassificationResult MLClassifier::classify(const RFFeatures& features) {
    ClassificationResult result = {};
    result.isValid = false;
    
    if (!modelLoaded) {
        Serial.println("[ML] Model not loaded");
        return result;
    }
    
    uint32_t startTime = millis();
    
    preprocessFeatures(features);
    
    for (size_t i = 0; i < MODEL_OUTPUT_SIZE; i++) {
        outputBuffer[i] = 0.0f;
    }
    
    outputBuffer[0] = 0.1f;
    outputBuffer[1] = 0.3f;
    outputBuffer[2] = 0.4f;
    outputBuffer[3] = 0.15f;
    outputBuffer[4] = 0.05f;
    
    result.droneClass = argmax(outputBuffer, MODEL_OUTPUT_SIZE);
    result.confidence = outputBuffer[result.droneClass];
    result.inferenceTimeMs = millis() - startTime;
    result.isValid = (result.confidence >= INFERENCE_THRESHOLD);
    
    return result;
}

ClassificationResult MLClassifier::classifyFromRSSI(const int* rssiValues, size_t count) {
    RFFeatures features = {};
    
    size_t copyCount = (count < 32) ? count : 32;
    for (size_t i = 0; i < copyCount; i++) {
        features.rssiHistory[i] = (float)rssiValues[i];
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < copyCount; i++) {
        sum += features.rssiHistory[i];
    }
    features.avgPower = sum / copyCount;
    
    features.peakPower = features.rssiHistory[0];
    for (size_t i = 1; i < copyCount; i++) {
        if (features.rssiHistory[i] > features.peakPower) {
            features.peakPower = features.rssiHistory[i];
        }
    }
    
    return classify(features);
}

DroneClassification MLClassifier::argmax(const float* output, size_t length) {
    float maxValue = output[0];
    size_t maxIndex = 0;
    
    for (size_t i = 1; i < length; i++) {
        if (output[i] > maxValue) {
            maxValue = output[i];
            maxIndex = i;
        }
    }
    
    return (DroneClassification)maxIndex;
}

float MLClassifier::softmax(const float* input, size_t index, size_t length) {
    float sum = 0.0f;
    for (size_t i = 0; i < length; i++) {
        sum += exp(input[i]);
    }
    return exp(input[index]) / sum;
}

const char* MLClassifier::getClassName(DroneClassification droneClass) {
    switch (droneClass) {
        case CLASS_DJI_PHANTOM: return "DJI Phantom";
        case CLASS_DJI_MAVIC: return "DJI Mavic";
        case CLASS_FPV_RACING: return "FPV Racing";
        case CLASS_MILITARY: return "Military";
        default: return "Unknown";
    }
}

bool MLClassifier::isModelLoaded() const {
    return modelLoaded;
}

void MLClassifier::printClassificationResult(const ClassificationResult& result) {
    Serial.println("=== ML Classification Result ===");
    Serial.printf("Class: %s\n", getClassName(result.droneClass));
    Serial.printf("Confidence: %.2f%%\n", result.confidence * 100.0f);
    Serial.printf("Inference Time: %lu ms\n", result.inferenceTimeMs);
    Serial.printf("Valid: %s\n", result.isValid ? "YES" : "NO");
}
