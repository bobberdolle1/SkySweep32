#include "config_manager.h"

// Global instance
ConfigManager configManager;

ConfigManager::ConfigManager() : spiffsReady(false) {
    setDefaults();
}

void ConfigManager::setDefaults() {
    // WiFi defaults from config.h
    strncpy(cfg.wifiSSID, WIFI_AP_SSID, sizeof(cfg.wifiSSID));
    strncpy(cfg.wifiPassword, WIFI_AP_PASSWORD, sizeof(cfg.wifiPassword));
    cfg.wifiAPMode = true;
    cfg.wifiChannel = WIFI_AP_CHANNEL;
    
    // RSSI thresholds from config.h
    cfg.rssiThresholdLow = RSSI_THRESHOLD_LOW;
    cfg.rssiThresholdMedium = RSSI_THRESHOLD_MEDIUM;
    cfg.rssiThresholdHigh = RSSI_THRESHOLD_HIGH;
    cfg.rssiThresholdCritical = RSSI_THRESHOLD_CRITICAL;
    
    // Scan intervals
    cfg.rfScanIntervalMs = RF_SCAN_INTERVAL_MS;
    cfg.displayUpdateMs = DISPLAY_UPDATE_INTERVAL_MS;
    cfg.bleScanIntervalMs = BLE_SCAN_INTERVAL_MS;
    
    
    // GPS
    cfg.gpsUpdateIntervalMs = GPS_UPDATE_INTERVAL;
    
    // Logging
    cfg.logLevel = 1;  // INFO
    cfg.maxLogSizeMB = MAX_LOG_SIZE_MB;
    
    // Stealth
    cfg.stealthMode = false;
}

bool ConfigManager::begin() {
    if (!SPIFFS.begin(true)) {  // true = format on fail
        Serial.println("[CONFIG] SPIFFS mount failed");
        spiffsReady = false;
        return false;
    }
    
    spiffsReady = true;
    Serial.printf("[CONFIG] SPIFFS: %u KB used / %u KB total\n",
                  SPIFFS.usedBytes() / 1024, SPIFFS.totalBytes() / 1024);
    
    // Try to load existing config
    if (SPIFFS.exists(CONFIG_FILE)) {
        if (load()) {
            Serial.println("[CONFIG] Loaded saved configuration");
            return true;
        }
        Serial.println("[CONFIG] Failed to parse config, using defaults");
    } else {
        Serial.println("[CONFIG] No saved config, using defaults");
        save();  // Save defaults so file exists
    }
    
    return true;
}

bool ConfigManager::load() {
    if (!spiffsReady) return false;
    
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("[CONFIG] Cannot open config file");
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("[CONFIG] Parse error: %s\n", error.c_str());
        return false;
    }
    
    // WiFi
    if (doc["wifi"]["ssid"]) strlcpy(cfg.wifiSSID, doc["wifi"]["ssid"], sizeof(cfg.wifiSSID));
    if (doc["wifi"]["password"]) strlcpy(cfg.wifiPassword, doc["wifi"]["password"], sizeof(cfg.wifiPassword));
    if (!doc["wifi"]["apMode"].isNull()) cfg.wifiAPMode = doc["wifi"]["apMode"];
    if (!doc["wifi"]["channel"].isNull()) cfg.wifiChannel = doc["wifi"]["channel"];
    
    // RSSI thresholds
    if (!doc["thresholds"]["low"].isNull()) cfg.rssiThresholdLow = doc["thresholds"]["low"];
    if (!doc["thresholds"]["medium"].isNull()) cfg.rssiThresholdMedium = doc["thresholds"]["medium"];
    if (!doc["thresholds"]["high"].isNull()) cfg.rssiThresholdHigh = doc["thresholds"]["high"];
    if (!doc["thresholds"]["critical"].isNull()) cfg.rssiThresholdCritical = doc["thresholds"]["critical"];
    
    // Scan intervals
    if (!doc["rfScanMs"].isNull()) cfg.rfScanIntervalMs = doc["rfScanMs"];
    if (!doc["displayMs"].isNull()) cfg.displayUpdateMs = doc["displayMs"];
    if (!doc["bleScanMs"].isNull()) cfg.bleScanIntervalMs = doc["bleScanMs"];
    
    
    // GPS
    if (!doc["gpsUpdateMs"].isNull()) cfg.gpsUpdateIntervalMs = doc["gpsUpdateMs"];
    
    // Logging
    if (!doc["logLevel"].isNull()) cfg.logLevel = doc["logLevel"];
    if (!doc["maxLogMB"].isNull()) cfg.maxLogSizeMB = doc["maxLogMB"];
    
    
    // Stealth
    if (!doc["stealthMode"].isNull()) cfg.stealthMode = doc["stealthMode"];
    
    return true;
}

bool ConfigManager::save() {
    if (!spiffsReady) return false;
    
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("[CONFIG] Cannot write config file");
        return false;
    }
    
    JsonDocument doc;
    
    // WiFi
    doc["wifi"]["ssid"] = cfg.wifiSSID;
    doc["wifi"]["password"] = cfg.wifiPassword;
    doc["wifi"]["apMode"] = cfg.wifiAPMode;
    doc["wifi"]["channel"] = cfg.wifiChannel;
    
    // RSSI thresholds
    doc["thresholds"]["low"] = cfg.rssiThresholdLow;
    doc["thresholds"]["medium"] = cfg.rssiThresholdMedium;
    doc["thresholds"]["high"] = cfg.rssiThresholdHigh;
    doc["thresholds"]["critical"] = cfg.rssiThresholdCritical;
    
    // Scan intervals
    doc["rfScanMs"] = cfg.rfScanIntervalMs;
    doc["displayMs"] = cfg.displayUpdateMs;
    doc["bleScanMs"] = cfg.bleScanIntervalMs;
    
    
    // GPS
    doc["gpsUpdateMs"] = cfg.gpsUpdateIntervalMs;
    
    // Logging
    doc["logLevel"] = cfg.logLevel;
    doc["maxLogMB"] = cfg.maxLogSizeMB;
    
    
    // Stealth
    doc["stealthMode"] = cfg.stealthMode;
    
    size_t written = serializeJsonPretty(doc, file);
    file.close();
    
    Serial.printf("[CONFIG] Saved (%u bytes)\n", written);
    return written > 0;
}

bool ConfigManager::reset() {
    setDefaults();
    return save();
}

bool ConfigManager::setWifi(const char* ssid, const char* password, bool apMode) {
    strlcpy(cfg.wifiSSID, ssid, sizeof(cfg.wifiSSID));
    strlcpy(cfg.wifiPassword, password, sizeof(cfg.wifiPassword));
    cfg.wifiAPMode = apMode;
    return save();
}

bool ConfigManager::setThresholds(uint8_t low, uint8_t med, uint8_t high, uint8_t crit) {
    cfg.rssiThresholdLow = low;
    cfg.rssiThresholdMedium = med;
    cfg.rssiThresholdHigh = high;
    cfg.rssiThresholdCritical = crit;
    return save();
}

bool ConfigManager::setScanInterval(uint32_t rfMs) {
    cfg.rfScanIntervalMs = rfMs;
    return save();
}


String ConfigManager::toJSON() const {
    JsonDocument doc;
    
    doc["wifi"]["ssid"] = cfg.wifiSSID;
    doc["wifi"]["apMode"] = cfg.wifiAPMode;
    doc["wifi"]["channel"] = cfg.wifiChannel;
    
    doc["thresholds"]["low"] = cfg.rssiThresholdLow;
    doc["thresholds"]["medium"] = cfg.rssiThresholdMedium;
    doc["thresholds"]["high"] = cfg.rssiThresholdHigh;
    doc["thresholds"]["critical"] = cfg.rssiThresholdCritical;
    
    doc["rfScanMs"] = cfg.rfScanIntervalMs;
    doc["displayMs"] = cfg.displayUpdateMs;
    doc["bleScanMs"] = cfg.bleScanIntervalMs;
    
    doc["logLevel"] = cfg.logLevel;
    
    doc["stealthMode"] = cfg.stealthMode;
    
    String output;
    serializeJson(doc, output);
    return output;
}

bool ConfigManager::fromJSON(const char* json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) return false;
    
    // Apply only fields that are present
    if (doc["wifi"]["ssid"]) strlcpy(cfg.wifiSSID, doc["wifi"]["ssid"], sizeof(cfg.wifiSSID));
    if (doc["wifi"]["password"]) strlcpy(cfg.wifiPassword, doc["wifi"]["password"], sizeof(cfg.wifiPassword));
    if (!doc["wifi"]["apMode"].isNull()) cfg.wifiAPMode = doc["wifi"]["apMode"];
    if (!doc["wifi"]["channel"].isNull()) cfg.wifiChannel = doc["wifi"]["channel"];
    
    if (!doc["thresholds"]["low"].isNull()) cfg.rssiThresholdLow = doc["thresholds"]["low"];
    if (!doc["thresholds"]["medium"].isNull()) cfg.rssiThresholdMedium = doc["thresholds"]["medium"];
    if (!doc["thresholds"]["high"].isNull()) cfg.rssiThresholdHigh = doc["thresholds"]["high"];
    if (!doc["thresholds"]["critical"].isNull()) cfg.rssiThresholdCritical = doc["thresholds"]["critical"];
    
    if (!doc["rfScanMs"].isNull()) cfg.rfScanIntervalMs = doc["rfScanMs"];
    if (!doc["logLevel"].isNull()) cfg.logLevel = doc["logLevel"];
    if (!doc["stealthMode"].isNull()) cfg.stealthMode = doc["stealthMode"];
    
    return save();
}

void ConfigManager::printConfig() const {
    Serial.println("=== Runtime Configuration ===");
    Serial.printf("WiFi SSID: %s (%s)\n", cfg.wifiSSID, cfg.wifiAPMode ? "AP" : "STA");
    Serial.printf("Thresholds: L=%d M=%d H=%d C=%d\n",
                  cfg.rssiThresholdLow, cfg.rssiThresholdMedium,
                  cfg.rssiThresholdHigh, cfg.rssiThresholdCritical);
    Serial.printf("RF Scan: %lu ms\n", cfg.rfScanIntervalMs);
    Serial.printf("Log Level: %d\n", cfg.logLevel);
    Serial.printf("Stealth Mode: %s\n", cfg.stealthMode ? "ON" : "OFF");
}
