#include "detection_model.h"

namespace {

uint8_t confidenceForSources(uint8_t sourceCount) {
    // Deliberately capped evidence score. It is not a probability calibration.
    return sourceCount == 0 ? 0 : sourceCount == 1 ? 35 : sourceCount == 2 ? 60 :
           sourceCount == 3 ? 78 : 88;
}

bool isCurrent(uint32_t now, uint32_t then, uint32_t window) {
    return now >= then && now - then <= window;
}

}  // namespace

DetectorFusion::DetectorFusion() : tracks_{}, currentDetection_{} {
    currentDetection_.classification = DetectionClassification::UNKNOWN;
    currentDetection_.level = RelativeLevel::NO_ACTIVITY;
    currentDetection_.trend = SignalTrend::STABLE;
}

RelativeLevel DetectorFusion::levelForNormalized(int16_t normalizedLevel) {
    if (normalizedLevel >= 75) return RelativeLevel::STRONG;
    if (normalizedLevel >= 55) return RelativeLevel::MEDIUM;
    if (normalizedLevel >= 35) return RelativeLevel::WEAK;
    if (normalizedLevel > 0) return RelativeLevel::VERY_WEAK;
    return RelativeLevel::NO_ACTIVITY;
}

const char* DetectorFusion::classificationName(DetectionClassification classification) {
    switch (classification) {
        case DetectionClassification::RF_ACTIVITY: return "RF_ACTIVITY";
        case DetectionClassification::RC_LINK_LIKE: return "RC_LINK_LIKE";
        case DetectionClassification::DIGITAL_LINK_LIKE: return "DIGITAL_LINK_LIKE";
        case DetectionClassification::DIGITAL_FPV_LIKE: return "DIGITAL_FPV_LIKE";
        case DetectionClassification::ANALOG_FPV_CONFIRMED:
            return "ANALOG_FPV_CONFIRMED";
        case DetectionClassification::REMOTE_ID_CONFIRMED: return "REMOTE_ID_CONFIRMED";
        default: return "UNKNOWN";
    }
}

const char* DetectorFusion::levelName(RelativeLevel level) {
    switch (level) {
        case RelativeLevel::VERY_WEAK: return "VERY_WEAK";
        case RelativeLevel::WEAK: return "WEAK";
        case RelativeLevel::MEDIUM: return "MEDIUM";
        case RelativeLevel::STRONG: return "STRONG";
        default: return "NO_ACTIVITY";
    }
}

const char* DetectorFusion::trendName(SignalTrend trend) {
    switch (trend) {
        case SignalTrend::APPROACHING: return "APPROACHING";
        case SignalTrend::RECEDING: return "RECEDING";
        default: return "STABLE";
    }
}

uint8_t DetectorFusion::indexFor(ObservationBand band) {
    return static_cast<uint8_t>(band);
}

uint32_t DetectorFusion::evidenceFlagFor(ObservationBand band) {
    switch (band) {
        case ObservationBand::SUB_GHZ_855_925: return EVIDENCE_SUB_GHZ_ACTIVITY;
        case ObservationBand::TWO_POINT_FOUR_GHZ: return EVIDENCE_TWO_POINT_FOUR_ACTIVITY;
        case ObservationBand::FIVE_X_LOW: return EVIDENCE_FIVE_X_LOW_ACTIVITY;
        case ObservationBand::FIVE_X_HIGH: return EVIDENCE_FIVE_X_HIGH_ACTIVITY;
        case ObservationBand::ANALOG_VIDEO: return EVIDENCE_ANALOG_VIDEO_SYNC;
        case ObservationBand::WIFI_BLE: return EVIDENCE_WIFI_BLE;
        case ObservationBand::REMOTE_ID: return EVIDENCE_REMOTE_ID;
        case ObservationBand::SKYPORT: return EVIDENCE_SKYPORT;
    }
    return EVIDENCE_NONE;
}

SignalTrend DetectorFusion::updateTrack(const BandObservation& observation) {
    Track& track = tracks_[indexFor(observation.band)];
    const int16_t level = observation.normalizedLevel < 0 ? 0 :
                          observation.normalizedLevel > 100 ? 100 : observation.normalizedLevel;
    const bool continued = track.active &&
                           isCurrent(observation.timestampMs, track.timestampMs,
                                     kCorrelationWindowMs);
    if (!continued) {
        track.firstTimestampMs = observation.timestampMs;
        track.filteredLevel = level;
        track.previousFilteredLevel = level;
    } else {
        track.previousFilteredLevel = track.filteredLevel;
        // Fixed-point EMA prevents an isolated RSSI sample becoming a trend.
        track.filteredLevel = static_cast<int16_t>((track.filteredLevel * 3 + level) / 4);
    }
    track.timestampMs = observation.timestampMs;
    track.dutyCyclePermille = observation.dutyCyclePermille;
    track.centerFrequencyMHz = observation.centerFrequencyMHz;
    track.active = observation.valid && levelForNormalized(level) != RelativeLevel::NO_ACTIVITY;

    const int16_t delta = track.filteredLevel - track.previousFilteredLevel;
    if (continued && delta >= 6) return SignalTrend::APPROACHING;
    if (continued && delta <= -6) return SignalTrend::RECEDING;
    return SignalTrend::STABLE;
}

Detection DetectorFusion::makeDetection(const BandObservation& primary, SignalTrend trend) const {
    Detection result = {};
    result.source = primary.band;
    result.timestampMs = primary.timestampMs;
    result.durationMs = primary.durationMs;
    result.level = primary.level;
    result.trend = trend;
    result.classification = DetectionClassification::UNKNOWN;

    uint32_t flags = 0;
    uint8_t sources = 0;
    for (uint8_t index = 0; index < sizeof(tracks_) / sizeof(tracks_[0]); ++index) {
        const Track& track = tracks_[index];
        if (!track.active || !isCurrent(primary.timestampMs, track.timestampMs,
                                        kCorrelationWindowMs)) {
            continue;
        }
        const ObservationBand band = static_cast<ObservationBand>(index);
        flags |= evidenceFlagFor(band);
        ++sources;
    }

    result.evidence.flags = flags;
    result.evidence.sourceCount = sources;
    result.evidence.confidence = confidenceForSources(sources);
    result.confidence = result.evidence.confidence;

    if ((flags & EVIDENCE_ANALOG_VIDEO_SYNC) != 0) {
        result.classification = DetectionClassification::ANALOG_FPV_CONFIRMED;
        result.confidence = 95;
    } else if ((flags & EVIDENCE_REMOTE_ID) != 0) {
        result.classification = DetectionClassification::REMOTE_ID_CONFIRMED;
        result.confidence = 95;
    } else if ((flags & EVIDENCE_TWO_POINT_FOUR_ACTIVITY) != 0 &&
               (flags & (EVIDENCE_FIVE_X_LOW_ACTIVITY | EVIDENCE_FIVE_X_HIGH_ACTIVITY)) != 0) {
        result.classification = DetectionClassification::DIGITAL_FPV_LIKE;
    } else if ((flags & (EVIDENCE_TWO_POINT_FOUR_ACTIVITY | EVIDENCE_SUB_GHZ_ACTIVITY)) != 0 &&
               sources >= 2) {
        result.classification = DetectionClassification::RC_LINK_LIKE;
    } else if ((flags & (EVIDENCE_FIVE_X_LOW_ACTIVITY | EVIDENCE_FIVE_X_HIGH_ACTIVITY)) != 0) {
        result.classification = DetectionClassification::DIGITAL_LINK_LIKE;
    } else if (sources > 0) {
        result.classification = DetectionClassification::RF_ACTIVITY;
    }
    return result;
}

Detection DetectorFusion::observe(const BandObservation& observation) {
    if (!observation.valid) return currentDetection_;
    BandObservation normalized = observation;
    normalized.level = levelForNormalized(normalized.normalizedLevel);
    const SignalTrend trend = updateTrack(normalized);
    currentDetection_ = makeDetection(normalized, trend);
    return currentDetection_;
}
