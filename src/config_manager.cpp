#include "config_manager.h"

#include <esp_system.h>

ConfigManager configManager;

namespace {
constexpr char kCredentialAlphabet[] =
    "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
constexpr size_t kApPasswordLength = 16;
constexpr char kRetiredPublicSsid[] = "SkySweep32";

bool copyBounded(char* destination, size_t destinationSize, const char* source) {
    if (!source || strnlen(source, destinationSize) >= destinationSize) return false;
    strlcpy(destination, source, destinationSize);
    return true;
}
}  // namespace


ConfigManager::ConfigManager() : spiffsReady(false), networkCredentialsGenerated(false) {
    setDefaults();
}

void ConfigManager::setDefaults() {
    cfg.wifiSSID[0] = '\0';
    cfg.wifiPassword[0] = '\0';
    cfg.wifiChannel = WIFI_AP_CHANNEL;
    cfg.rssiThresholdLow = RSSI_THRESHOLD_LOW;
    cfg.rssiThresholdMedium = RSSI_THRESHOLD_MEDIUM;
    cfg.rssiThresholdHigh = RSSI_THRESHOLD_HIGH;
    cfg.rssiThresholdCritical = RSSI_THRESHOLD_CRITICAL;
    cfg.rfScanIntervalMs = RF_SCAN_INTERVAL_MS;
    cfg.displayUpdateMs = DISPLAY_UPDATE_INTERVAL_MS;
    cfg.bleScanIntervalMs = BLE_SCAN_INTERVAL_MS;
    cfg.gpsUpdateIntervalMs = GPS_UPDATE_INTERVAL;
    cfg.logLevel = 1;
    cfg.maxLogSizeMB = MAX_LOG_SIZE_MB;
    cfg.stealthMode = false;
}

void ConfigManager::generateNetworkCredentials() {
    const uint64_t mac = ESP.getEfuseMac();
    snprintf(cfg.wifiSSID, sizeof(cfg.wifiSSID), "%s-%04llX", WIFI_AP_SSID_PREFIX,
             static_cast<unsigned long long>(mac & 0xffffULL));
    for (size_t index = 0; index < kApPasswordLength; ++index) {
        cfg.wifiPassword[index] =
            kCredentialAlphabet[esp_random() % (sizeof(kCredentialAlphabet) - 1)];
    }
    cfg.wifiPassword[kApPasswordLength] = '\0';
    networkCredentialsGenerated = true;
}

bool ConfigManager::validateNetworkConfig(const RuntimeConfig& candidate) const {
    const size_t ssidLength = strnlen(candidate.wifiSSID, sizeof(candidate.wifiSSID));
    const size_t passwordLength =
        strnlen(candidate.wifiPassword, sizeof(candidate.wifiPassword));
    return ssidLength >= 1 && ssidLength < sizeof(candidate.wifiSSID) &&
           passwordLength >= 8 && passwordLength < sizeof(candidate.wifiPassword) &&
           candidate.wifiChannel >= 1 && candidate.wifiChannel <= 13;
}

bool ConfigManager::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("[CONFIG] SPIFFS mount failed");
        spiffsReady = false;
        return false;
    }

    spiffsReady = true;
    Serial.printf("[CONFIG] SPIFFS: %u KB used / %u KB total\n",
                  SPIFFS.usedBytes() / 1024, SPIFFS.totalBytes() / 1024);
    if (SPIFFS.exists(CONFIG_FILE) && load()) {
        Serial.println("[CONFIG] Loaded saved configuration");
        return true;
    }

    setDefaults();
    generateNetworkCredentials();
    if (!save()) return false;
    Serial.println("[CONFIG] Generated first-boot AP credentials");
    Serial.printf("[CONFIG] AP SSID: %s\n", cfg.wifiSSID);
    Serial.printf("[CONFIG] AP password: %s\n", cfg.wifiPassword);
    return true;
}

bool ConfigManager::load() {
    if (!spiffsReady) return false;
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) return false;

    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error || !doc.is<JsonObject>()) return false;

    RuntimeConfig loaded = cfg;
    const JsonVariantConst wifi = doc["wifi"];
    if (!wifi.is<JsonObject>()) return false;
    const JsonObjectConst wifiObject = wifi.as<JsonObjectConst>();
    const JsonVariantConst ssid = wifiObject["ssid"];
    const JsonVariantConst password = wifiObject["password"];
    const JsonVariantConst channel = wifiObject["channel"];
    if (!ssid.is<const char*>() || !password.is<const char*>() || !channel.is<int>()) {
        return false;
    }
    if (!copyBounded(loaded.wifiSSID, sizeof(loaded.wifiSSID), ssid.as<const char*>()) ||
        !copyBounded(loaded.wifiPassword, sizeof(loaded.wifiPassword),
                     password.as<const char*>())) {
        return false;
    }
    const int configuredChannel = channel.as<int>();
    if (configuredChannel < 1 || configuredChannel > 13) return false;
    loaded.wifiChannel = static_cast<uint8_t>(configuredChannel);

    if (!doc["thresholds"]["low"].isNull()) loaded.rssiThresholdLow = doc["thresholds"]["low"];
    if (!doc["thresholds"]["medium"].isNull()) loaded.rssiThresholdMedium = doc["thresholds"]["medium"];
    if (!doc["thresholds"]["high"].isNull()) loaded.rssiThresholdHigh = doc["thresholds"]["high"];
    if (!doc["thresholds"]["critical"].isNull()) loaded.rssiThresholdCritical = doc["thresholds"]["critical"];
    if (!doc["rfScanMs"].isNull()) loaded.rfScanIntervalMs = doc["rfScanMs"];
    if (!doc["displayMs"].isNull()) loaded.displayUpdateMs = doc["displayMs"];
    if (!doc["bleScanMs"].isNull()) loaded.bleScanIntervalMs = doc["bleScanMs"];
    if (!doc["gpsUpdateMs"].isNull()) loaded.gpsUpdateIntervalMs = doc["gpsUpdateMs"];
    if (!doc["logLevel"].isNull()) loaded.logLevel = doc["logLevel"];
    if (!doc["maxLogMB"].isNull()) loaded.maxLogSizeMB = doc["maxLogMB"];
    if (!doc["stealthMode"].isNull()) loaded.stealthMode = doc["stealthMode"];

    if (!validateNetworkConfig(loaded)) return false;
    cfg = loaded;
    if (strcmp(cfg.wifiSSID, kRetiredPublicSsid) == 0) {
        generateNetworkCredentials();
        return save();
    }
    return true;
}

bool ConfigManager::save() {
    if (!spiffsReady) return false;
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) return false;

    JsonDocument doc;
    doc["wifi"]["ssid"] = cfg.wifiSSID;
    doc["wifi"]["password"] = cfg.wifiPassword;
    doc["wifi"]["channel"] = cfg.wifiChannel;
    doc["thresholds"]["low"] = cfg.rssiThresholdLow;
    doc["thresholds"]["medium"] = cfg.rssiThresholdMedium;
    doc["thresholds"]["high"] = cfg.rssiThresholdHigh;
    doc["thresholds"]["critical"] = cfg.rssiThresholdCritical;
    doc["rfScanMs"] = cfg.rfScanIntervalMs;
    doc["displayMs"] = cfg.displayUpdateMs;
    doc["bleScanMs"] = cfg.bleScanIntervalMs;
    doc["gpsUpdateMs"] = cfg.gpsUpdateIntervalMs;
    doc["logLevel"] = cfg.logLevel;
    doc["maxLogMB"] = cfg.maxLogSizeMB;
    doc["stealthMode"] = cfg.stealthMode;
    const size_t written = serializeJsonPretty(doc, file);
    file.close();
    Serial.printf("[CONFIG] Saved (%u bytes)\n", written);
    return written > 0;
}

bool ConfigManager::reset() {
    if (!spiffsReady) return false;
    networkCredentialsGenerated = false;
    // Keep the active credential in RAM until reboot so this authenticated
    // request cannot weaken management access before the new AP is generated.
    // The next boot has no file and therefore creates/displays a new secret.
    return !SPIFFS.exists(CONFIG_FILE) || SPIFFS.remove(CONFIG_FILE);
}

bool ConfigManager::setWifi(const char* ssid, const char* password, uint8_t channel) {
    RuntimeConfig updated = cfg;
    if (!copyBounded(updated.wifiSSID, sizeof(updated.wifiSSID), ssid) ||
        !copyBounded(updated.wifiPassword, sizeof(updated.wifiPassword), password)) {
        return false;
    }
    updated.wifiChannel = channel;
    if (!validateNetworkConfig(updated)) return false;
    cfg = updated;
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
    const DeserializationError error = deserializeJson(doc, json);
    if (error || !doc.is<JsonObject>()) return false;

    RuntimeConfig updated = cfg;
    const JsonVariantConst wifi = doc["wifi"];
    if (!wifi.isNull()) {
        if (!wifi.is<JsonObject>()) return false;
        const JsonObjectConst wifiObject = wifi.as<JsonObjectConst>();
        if (!wifiObject["apMode"].isNull()) return false;
        if (!wifiObject["ssid"].isNull()) {
            if (!wifiObject["ssid"].is<const char*>() ||
                !copyBounded(updated.wifiSSID, sizeof(updated.wifiSSID),
                             wifiObject["ssid"].as<const char*>())) {
                return false;
            }
        }
        if (!wifiObject["password"].isNull()) {
            if (!wifiObject["password"].is<const char*>() ||
                !copyBounded(updated.wifiPassword, sizeof(updated.wifiPassword),
                             wifiObject["password"].as<const char*>())) {
                return false;
            }
        }
        if (!wifiObject["channel"].isNull()) {
            if (!wifiObject["channel"].is<int>()) return false;
            const int channel = wifiObject["channel"].as<int>();
            if (channel < 1 || channel > 13) return false;
            updated.wifiChannel = static_cast<uint8_t>(channel);
        }
    }
    if (!validateNetworkConfig(updated)) return false;

    if (!doc["thresholds"]["low"].isNull()) updated.rssiThresholdLow = doc["thresholds"]["low"];
    if (!doc["thresholds"]["medium"].isNull()) updated.rssiThresholdMedium = doc["thresholds"]["medium"];
    if (!doc["thresholds"]["high"].isNull()) updated.rssiThresholdHigh = doc["thresholds"]["high"];
    if (!doc["thresholds"]["critical"].isNull()) updated.rssiThresholdCritical = doc["thresholds"]["critical"];
    if (!doc["rfScanMs"].isNull()) updated.rfScanIntervalMs = doc["rfScanMs"];
    if (!doc["logLevel"].isNull()) updated.logLevel = doc["logLevel"];
    if (!doc["stealthMode"].isNull()) updated.stealthMode = doc["stealthMode"];
    cfg = updated;
    return save();
}

void ConfigManager::printConfig() const {
    Serial.println("=== Runtime Configuration ===");
    Serial.printf("AP SSID: %s (channel %u)\n", cfg.wifiSSID, cfg.wifiChannel);
    Serial.printf("Thresholds: L=%d M=%d H=%d C=%d\n", cfg.rssiThresholdLow,
                  cfg.rssiThresholdMedium, cfg.rssiThresholdHigh,
                  cfg.rssiThresholdCritical);
    Serial.printf("RF Scan: %lu ms\n", cfg.rfScanIntervalMs);
    Serial.printf("Log Level: %d\n", cfg.logLevel);
    Serial.printf("Stealth Mode: %s\n", cfg.stealthMode ? "ON" : "OFF");
}
