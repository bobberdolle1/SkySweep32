#ifndef COUNTERMEASURES_H
#define COUNTERMEASURES_H

#include <Arduino.h>
#include <SPI.h>
#include "config.h"

// Threat classification
enum ThreatLevel {
    THREAT_NONE = 0,
    THREAT_LOW = 1,      // Weak signal, distant
    THREAT_MEDIUM = 2,   // Moderate signal, approaching
    THREAT_HIGH = 3,     // Strong signal, imminent
    THREAT_CRITICAL = 4  // Very strong signal, immediate danger
};

// Countermeasure types
enum CountermeasureType {
    CM_NONE = 0,
    CM_JAMMING_BROADBAND = 1,    // Wideband noise injection (NRF/CC1101)
    CM_JAMMING_TARGETED = 2,     // Protocol-specific jamming (NRF/CC1101)
    CM_PROTOCOL_INJECTION = 3,   // Command hijacking (RTH/Land)
    CM_DEAUTH_FLOOD = 4,         // Deauthentication attack (2.4GHz Wi-Fi)
    CM_VCO_VIDEO_JAMMING = 5,    // 5.8GHz DAC Sweep for Video (Analog/Digital)
    CM_VCO_CONTROL_JAMMING = 6,  // 900MHz/2.4GHz DAC Sweep for Control (ELRS)
    CM_VCO_GPS_JAMMING = 7       // 1.5GHz GPS Denial
};

// Drone protocol signatures
enum DroneProtocol {
    PROTOCOL_UNKNOWN = 0,
    PROTOCOL_DJI_LIGHTBRIDGE = 1,    // 2.4/5.8 GHz
    PROTOCOL_DJI_OCUSYNC = 2,        // 2.4/5.8 GHz
    PROTOCOL_MAVLINK = 3,            // 433/900 MHz
    PROTOCOL_ELRS = 4,               // 900 MHz/2.4 GHz
    PROTOCOL_CROSSFIRE = 5,          // 900 MHz
    PROTOCOL_ANALOG_VIDEO = 6,       // 5.8 GHz
    PROTOCOL_WALKSNAIL = 7,          // 5.8 GHz Digital
    PROTOCOL_BETAFPV_ARTLYNK = 8,    // 5.8 GHz Digital
    PROTOCOL_OPENIPC = 9             // 5.8/2.4 GHz Wi-Fi Raw
};

// Threat assessment data
struct ThreatData {
    uint8_t moduleIndex;
    int rssiValue;
    ThreatLevel level;
    DroneProtocol detectedProtocol;
    uint32_t firstDetectionTime;
    uint32_t lastUpdateTime;
    bool isActive;
};

class CountermeasureSystem {
private:
    ThreatData currentThreat;
    bool systemArmed;
    uint8_t powerAmplifierEnabled;
    
    // Protocol detection
    DroneProtocol analyzeSignalPattern(uint8_t moduleIndex, int rssi);
    
    // Jamming functions
    void executeWidebandjamming(uint8_t chipSelectPin, uint32_t frequency);
    void executeTargetedJamming(uint8_t chipSelectPin, DroneProtocol protocol);
    
    // Protocol injection
    void injectMAVLinkRTH(uint8_t chipSelectPin);
    void injectDJIEmergencyLand(uint8_t chipSelectPin);
    void executeDeauthFlood(uint8_t chipSelectPin);
    
    // VCO Juggernaut operations
    void executeVCOJamming(CountermeasureType type);
    void sweepDAC(uint8_t dacPin, uint32_t duration, uint16_t minVoltage, uint16_t maxVoltage);
    
public:
    CountermeasureSystem();
    
    // System control
    void initialize();
    void armSystem(bool armed);
    bool isArmed() const { return systemArmed; }
    
    // Threat assessment
    ThreatLevel assessThreat(uint8_t moduleIndex, int rssiValue);
    ThreatData getCurrentThreat() const { return currentThreat; }
    
    // Countermeasure execution
    bool executeCountermeasure(CountermeasureType type, uint8_t chipSelectPin);
    
    // Automatic response
    void autoRespond(uint8_t moduleIndex, int rssiValue, uint8_t chipSelectPin);
    
    // Utility
    const char* getThreatLevelString(ThreatLevel level);
    const char* getProtocolString(DroneProtocol protocol);
};

#endif // COUNTERMEASURES_H
