#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

#define CONFIG_FILE "/config.json"

// Runtime-configurable settings (stored in SPIFFS)
struct RuntimeConfig {
    // AP network and its management credential. Rev C intentionally supports
    // AP mode only until STA behavior has physical bring-up evidence.
    char wifiSSID[32];
    char wifiPassword[64];
    uint8_t wifiChannel;
    // RSSI thresholds (override config.h defaults)
    uint8_t rssiThresholdLow;
    uint8_t rssiThresholdMedium;
    uint8_t rssiThresholdHigh;
    uint8_t rssiThresholdCritical;
    
    // Scan intervals
    uint32_t rfScanIntervalMs;
    uint32_t displayUpdateMs;
    uint32_t bleScanIntervalMs;
    
    
    // GPS
    uint32_t gpsUpdateIntervalMs;
    
    // Logging
    uint8_t logLevel;  // 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR
    uint32_t maxLogSizeMB;
    
    // Stealth
    bool stealthMode;
};

class ConfigManager {
private:
    RuntimeConfig cfg;
    bool spiffsReady;
    bool networkCredentialsGenerated;

    void setDefaults();
    void generateNetworkCredentials();
    bool validateNetworkConfig(const RuntimeConfig& candidate) const;

public:
    ConfigManager();
    
    bool begin();
    bool load();
    bool save();
    bool reset();

    RuntimeConfig& get() { return cfg; }
    const RuntimeConfig& get() const { return cfg; }
    bool networkCredentialsWereGenerated() const { return networkCredentialsGenerated; }

    // Convenience setters with auto-save
    bool setWifi(const char* ssid, const char* password, uint8_t channel);
    bool setThresholds(uint8_t low, uint8_t med, uint8_t high, uint8_t crit);
    bool setScanInterval(uint32_t rfMs);
    
    // JSON export/import (for web API)
    String toJSON() const;
    bool fromJSON(const char* json);
    
    void printConfig() const;
};

// Global instance
extern ConfigManager configManager;

#endif // CONFIG_MANAGER_H
