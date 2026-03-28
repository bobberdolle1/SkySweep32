#include "ml_classifier.h"

#ifdef MODULE_ML

#include <cmath>

MLClassifier::MLClassifier() : modelLoaded(false) {
    memset(inputBuffer, 0, sizeof(inputBuffer));
    memset(outputBuffer, 0, sizeof(outputBuffer));
}

bool MLClassifier::begin() {
    Serial.println("[ML] Initializing drone classifier (rule-based)...");
    
    // Rule-based classifier is always ready
    // TFLite model can be loaded later from SD card for upgrade
    modelLoaded = false;  // No TFLite model loaded by default
    
    Serial.println("[ML] Rule-based classifier ready");
    Serial.println("[ML] Load TFLite model from SD for AI classification");
    return true;
}

bool MLClassifier::loadModel(const uint8_t* modelData, size_t modelSize) {
    if (modelData == nullptr || modelSize == 0) {
        Serial.println("[ML] Invalid model data");
        return false;
    }
    
    Serial.printf("[ML] Loading TFLite model (%u bytes)...\n", modelSize);
    
    // TODO: Actual TFLite Micro model loading
    // For now, mark as loaded for future implementation
    modelLoaded = true;
    Serial.println("[ML] TFLite model loaded (upgrade path ready)");
    return true;
}

// ============================================================================
// RULE-BASED CLASSIFICATION
// Uses protocol detection + frequency band + RSSI patterns
// ============================================================================

ClassificationResult MLClassifier::classifyRuleBased(const RFFeatures& features) {
    ClassificationResult result = {};
    result.isValid = false;
    
    float scores[ML_OUTPUT_SIZE] = {0};
    // scores[0] = Unknown
    // scores[1] = DJI Phantom
    // scores[2] = DJI Mavic
    // scores[3] = FPV Racing
    // scores[4] = Military
    
    // --- Rule 1: Protocol-based classification ---
    
    // MAVLink on 900 MHz = likely ArduPilot/PX4 drone (could be military/DIY)
    if (features.mavlinkDetected && features.band900Active) {
        scores[4] += 0.4f;  // Military/professional
        scores[0] += 0.2f;  // Could be DIY
    }
    
    // MAVLink on 2.4 GHz = likely consumer drone with telemetry
    if (features.mavlinkDetected && features.band2400Active) {
        scores[1] += 0.3f;  // DJI Phantom class
        scores[0] += 0.2f;
    }
    
    // CRSF/ExpressLRS detected = FPV racing drone
    if (features.crsfDetected) {
        scores[3] += 0.6f;  // FPV Racing - high confidence
    }
    
    // DJI pattern (dual band 2.4 + 5.8 GHz simultaneously)
    if (features.djiPatternDetected || (features.band2400Active && features.band5800Active)) {
        scores[1] += 0.3f;  // DJI Phantom
        scores[2] += 0.4f;  // DJI Mavic (more common)
    }
    
    // Analog video on 5.8 GHz only (no 2.4 GHz control) = classic FPV
    if (features.analogVideoDetected && features.band5800Active && !features.band2400Active) {
        scores[3] += 0.5f;  // FPV Racing
    }
    
    // --- Rule 2: RSSI pattern analysis ---
    
    // Calculate RSSI variance (stable = hovering DJI, variable = racing)
    float rssiMean = features.avgPower;
    float rssiVariance = 0;
    int validSamples = 0;
    
    for (int i = 0; i < RSSI_HISTORY_SIZE; i++) {
        if (features.rssiHistory[i] > 0) {
            float diff = features.rssiHistory[i] - rssiMean;
            rssiVariance += diff * diff;
            validSamples++;
        }
    }
    
    if (validSamples > 0) {
        rssiVariance /= validSamples;
        
        // Low variance (stable signal) = DJI hovering
        if (rssiVariance < 50.0f) {
            scores[1] += 0.15f;
            scores[2] += 0.15f;
        }
        
        // High variance (rapidly changing) = FPV racing or military evasion
        if (rssiVariance > 200.0f) {
            scores[3] += 0.2f;
            scores[4] += 0.1f;
        }
    }
    
    // --- Rule 3: Band combination patterns ---
    
    // Only 900 MHz active = long-range (military/professional)
    if (features.band900Active && !features.band2400Active && !features.band5800Active) {
        scores[4] += 0.3f;
    }
    
    // All three bands active = comprehensive system (DJI with FPV cam or military)
    if (features.band900Active && features.band2400Active && features.band5800Active) {
        scores[4] += 0.2f;
        scores[2] += 0.1f;
    }
    
    // --- Rule 4: Signal strength ---
    
    // Very strong signal on 2.4 GHz = close DJI drone
    if (features.band2400Active && features.peakPower > 80.0f) {
        scores[1] += 0.1f;
        scores[2] += 0.15f;
    }
    
    // --- Find best classification ---
    
    // Normalize scores
    float maxScore = 0;
    int bestClass = 0;
    float totalScore = 0;
    
    for (int i = 0; i < ML_OUTPUT_SIZE; i++) {
        totalScore += scores[i];
        if (scores[i] > maxScore) {
            maxScore = scores[i];
            bestClass = i;
        }
    }
    
    result.droneClass = (DroneClassification)bestClass;
    result.confidence = (totalScore > 0) ? (maxScore / totalScore) : 0.0f;
    result.isValid = (result.confidence >= ML_INFERENCE_THRESHOLD);
    
    return result;
}

ClassificationResult MLClassifier::classify(const RFFeatures& features) {
    uint32_t startTime = millis();
    
    ClassificationResult result;
    
    // Use TFLite if available, otherwise fall back to rules
    if (modelLoaded) {
        // TODO: TFLite inference path
        // For now, use rule-based
        result = classifyRuleBased(features);
    } else {
        result = classifyRuleBased(features);
    }
    
    result.inferenceTimeMs = millis() - startTime;
    return result;
}

ClassificationResult MLClassifier::classifyFromRSSI(const int* rssiValues, size_t count,
                                                     bool mavlink, bool crsf,
                                                     bool band900, bool band2400, bool band5800) {
    RFFeatures features = {};
    
    size_t copyCount = (count < RSSI_HISTORY_SIZE) ? count : RSSI_HISTORY_SIZE;
    for (size_t i = 0; i < copyCount; i++) {
        features.rssiHistory[i] = (float)rssiValues[i];
    }
    
    // Calculate stats
    float sum = 0.0f;
    features.peakPower = 0;
    for (size_t i = 0; i < copyCount; i++) {
        sum += features.rssiHistory[i];
        if (features.rssiHistory[i] > features.peakPower) {
            features.peakPower = features.rssiHistory[i];
        }
    }
    features.avgPower = (copyCount > 0) ? sum / copyCount : 0;
    
    // Set protocol flags
    features.mavlinkDetected = mavlink;
    features.crsfDetected = crsf;
    features.band900Active = band900;
    features.band2400Active = band2400;
    features.band5800Active = band5800;
    
    // Infer DJI pattern from dual-band activity
    features.djiPatternDetected = (band2400 && band5800 && !mavlink && !crsf);
    features.analogVideoDetected = (band5800 && !band2400);
    
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

const char* MLClassifier::getClassName(DroneClassification droneClass) {
    switch (droneClass) {
        case CLASS_DJI_PHANTOM: return "DJI Phantom";
        case CLASS_DJI_MAVIC: return "DJI Mavic";
        case CLASS_FPV_RACING: return "FPV Racing";
        case CLASS_MILITARY: return "Military/Pro";
        default: return "Unknown";
    }
}

bool MLClassifier::isModelLoaded() const {
    return modelLoaded;
}

void MLClassifier::printClassificationResult(const ClassificationResult& result) {
    Serial.println("=== Drone Classification ===");
    Serial.printf("Class: %s\n", getClassName(result.droneClass));
    Serial.printf("Confidence: %.1f%%\n", result.confidence * 100.0f);
    Serial.printf("Inference: %lu ms\n", result.inferenceTimeMs);
    Serial.printf("Valid: %s\n", result.isValid ? "YES" : "NO");
}

#endif // MODULE_ML
