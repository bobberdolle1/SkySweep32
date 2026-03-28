#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

#define CONFIG_FILE "/config.json"

// Runtime-configurable settings (stored in SPIFFS)
struct RuntimeConfig {
    // WiFi
    char wifiSSID[32];
    char wifiPassword[64];
    bool wifiAPMode;
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
    
    // LoRa
    float loraFrequency;
    uint8_t loraTxPower;
    uint32_t loraTransmitIntervalMs;
    
    // GPS
    uint32_t gpsUpdateIntervalMs;
    
    // Logging
    uint8_t logLevel;  // 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR
    uint32_t maxLogSizeMB;
    
    // Countermeasures
    bool countermeasuresArmed;
};

class ConfigManager {
private:
    RuntimeConfig cfg;
    bool spiffsReady;
    
    void setDefaults();

public:
    ConfigManager();
    
    bool begin();
    bool load();
    bool save();
    bool reset();
    
    RuntimeConfig& get() { return cfg; }
    const RuntimeConfig& get() const { return cfg; }
    
    // Convenience setters with auto-save
    bool setWifi(const char* ssid, const char* password, bool apMode);
    bool setThresholds(uint8_t low, uint8_t med, uint8_t high, uint8_t crit);
    bool setScanInterval(uint32_t rfMs);
    bool setLoRa(float freq, uint8_t txPower);
    
    // JSON export/import (for web API)
    String toJSON() const;
    bool fromJSON(const char* json);
    
    void printConfig() const;
};

// Global instance
extern ConfigManager configManager;

#endif // CONFIG_MANAGER_H
