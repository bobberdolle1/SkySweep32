#ifndef SIGNAL_DATABASE_H
#define SIGNAL_DATABASE_H

#include <Arduino.h>
#include "config.h"

#define MAX_SIGNATURES      32
#define SIGNATURE_NAME_LEN  24

// Frequency band identifiers
enum RFBand {
    RF_BAND_433  = 0,
    RF_BAND_868  = 1,
    RF_BAND_915  = 2,
    RF_BAND_2400 = 3,
    RF_BAND_5800 = 4
};

// Known drone RF signature
struct DroneSignature {
    char name[SIGNATURE_NAME_LEN];     // e.g. "DJI Mini 3 Pro"
    
    // Band activity pattern
    bool usesband[5];                  // Which bands are active [433,868,915,2400,5800]
    
    // RSSI characteristics
    int8_t  typicalRssiMin;            // Minimum expected RSSI
    int8_t  typicalRssiMax;            // Maximum expected RSSI
    float   rssiVarianceThreshold;     // Variance signature (hovering vs moving)
    
    // Protocol
    uint8_t protocol;                  // DroneProtocol enum match
    bool    hasMavlink;
    bool    hasCrsf;
    bool    hasRemoteID;
    
    // Channel hopping pattern
    uint8_t hopPatternLength;          // 0 = no hopping, >0 = known pattern
    uint8_t hopChannels[8];            // First channels of hop pattern
    
    // Classification
    uint8_t droneClass;                // 0=consumer, 1=fpv, 2=commercial, 3=military
    char    manufacturer[16];
    
    bool    active;                    // Entry in use
};

// Match result
struct SignatureMatch {
    uint8_t signatureIndex;
    float   confidence;                // 0.0-1.0
    const char* name;
    uint8_t droneClass;
};

class SignalDatabase {
private:
    DroneSignature db[MAX_SIGNATURES];
    uint8_t count;
    
    void loadDefaults();
    float calculateMatchScore(const DroneSignature& sig,
                               bool bands[5], int8_t rssi, float variance,
                               bool mavlink, bool crsf, bool remoteID);

public:
    SignalDatabase();
    
    void begin();
    
    // Add/manage signatures
    bool addSignature(const DroneSignature& sig);
    bool removeSignature(uint8_t index);
    uint8_t getCount() const { return count; }
    const DroneSignature* getSignature(uint8_t index) const;
    
    // Match incoming signal against database
    SignatureMatch matchSignal(bool activeBands[5], int8_t rssi, float rssiVariance,
                               bool mavlink, bool crsf, bool remoteID);
    
    // Best match across recent history
    SignatureMatch matchFromHistory(int8_t* rssiHistory, uint16_t histLen,
                                     bool activeBands[5], bool mavlink, bool crsf);
    
    // Export/import for persistence
    String toJSON() const;
    bool fromJSON(const char* json);
    
    // Save/load to SPIFFS
    bool save();
    bool load();
};

extern SignalDatabase signalDB;

#endif // SIGNAL_DATABASE_H
