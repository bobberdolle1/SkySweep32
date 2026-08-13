#ifndef DETECTION_MODEL_H
#define DETECTION_MODEL_H

#include <stdint.h>

// Evidence is intentionally source-specific. A relative receiver level is not
// protocol identity, aircraft identity, range, transmitter power, or bearing.
enum class ObservationBand : uint8_t {
    SUB_GHZ_855_925,
    TWO_POINT_FOUR_GHZ,
    FIVE_X_LOW,
    FIVE_X_HIGH,
    ANALOG_VIDEO,
    WIFI_BLE,
    REMOTE_ID,
    SKYPORT,
};

enum class RelativeLevel : uint8_t {
    NO_ACTIVITY,
    VERY_WEAK,
    WEAK,
    MEDIUM,
    STRONG,
};

enum class SignalTrend : uint8_t {
    STABLE,
    APPROACHING,
    RECEDING,
};

enum class DetectionClassification : uint8_t {
    UNKNOWN,
    RF_ACTIVITY,
    RC_LINK_LIKE,
    DIGITAL_LINK_LIKE,
    DIGITAL_FPV_LIKE,
    ANALOG_FPV_CONFIRMED,
    REMOTE_ID_CONFIRMED,
};

enum DetectionEvidenceFlag : uint32_t {
    EVIDENCE_NONE = 0,
    EVIDENCE_SUB_GHZ_ACTIVITY = 1u << 0,
    EVIDENCE_TWO_POINT_FOUR_ACTIVITY = 1u << 1,
    EVIDENCE_FIVE_X_LOW_ACTIVITY = 1u << 2,
    EVIDENCE_FIVE_X_HIGH_ACTIVITY = 1u << 3,
    EVIDENCE_ANALOG_VIDEO_SYNC = 1u << 4,
    EVIDENCE_WIFI_BLE = 1u << 5,
    EVIDENCE_REMOTE_ID = 1u << 6,
    EVIDENCE_SKYPORT = 1u << 7,
};

struct BandObservation {
    ObservationBand band;
    uint32_t timestampMs;
    uint16_t centerFrequencyMHz;  // 0 when the source has no frequency bin.
    uint16_t durationMs;
    uint16_t dutyCyclePermille;
    int16_t normalizedLevel;      // source-normalized 0..100, never dBm.
    RelativeLevel level;
    bool valid;
};

struct SignalFeatures {
    uint16_t durationMs;
    uint16_t dutyCyclePermille;
    uint16_t peakLevel;
    SignalTrend trend;
};

struct DetectionEvidence {
    uint32_t flags;
    uint8_t sourceCount;
    uint8_t confidence;  // Evidence score, not probability or protocol proof.
};

struct Detection {
    ObservationBand source;
    RelativeLevel level;
    uint32_t timestampMs;
    uint32_t durationMs;
    uint8_t confidence;
    SignalTrend trend;
    DetectionClassification classification;
    DetectionEvidence evidence;
};

class DetectorFusion {
public:
    DetectorFusion();

    // Ingest exactly one already-normalized source observation. Observations
    // outside the correlation window cannot corroborate a current detection.
    Detection observe(const BandObservation& observation);
    Detection current() const { return currentDetection_; }

    static RelativeLevel levelForNormalized(int16_t normalizedLevel);
    static const char* classificationName(DetectionClassification classification);
    static const char* levelName(RelativeLevel level);
    static const char* trendName(SignalTrend trend);

private:
    struct Track {
        uint32_t timestampMs;
        uint32_t firstTimestampMs;
        int16_t filteredLevel;
        int16_t previousFilteredLevel;
        uint16_t dutyCyclePermille;
        uint16_t centerFrequencyMHz;
        bool active;
    };

    static const uint32_t kCorrelationWindowMs = 3000;
    Track tracks_[8];
    Detection currentDetection_;

    static uint8_t indexFor(ObservationBand band);
    static uint32_t evidenceFlagFor(ObservationBand band);
    SignalTrend updateTrack(const BandObservation& observation);
    Detection makeDetection(const BandObservation& primary, SignalTrend trend) const;
};

#endif  // DETECTION_MODEL_H
