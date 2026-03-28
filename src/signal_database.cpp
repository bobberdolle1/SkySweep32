#include "signal_database.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <math.h>

#define SIGDB_FILE "/sigdb.json"

SignalDatabase signalDB;

SignalDatabase::SignalDatabase() : count(0) {
    memset(db, 0, sizeof(db));
}

void SignalDatabase::begin() {
    // Try loading from SPIFFS first
    if (load()) {
        Serial.printf("[SIGDB] Loaded %d signatures from flash\n", count);
    } else {
        // Load built-in defaults
        loadDefaults();
        Serial.printf("[SIGDB] Loaded %d default signatures\n", count);
    }
}

// --- Built-in drone signatures ---

void SignalDatabase::loadDefaults() {
    count = 0;
    
    // DJI series (2.4 + 5.8 GHz, OcuSync/Lightbridge)
    {
        DroneSignature sig = {};
        strlcpy(sig.name, "DJI (OcuSync)", SIGNATURE_NAME_LEN);
        sig.usesband[RF_BAND_2400] = true;
        sig.usesband[RF_BAND_5800] = true;
        sig.typicalRssiMin = -80;
        sig.typicalRssiMax = -20;
        sig.rssiVarianceThreshold = 5.0f;
        sig.protocol = 2;  // PROTOCOL_DJI_OCUSYNC
        sig.hasMavlink = false;
        sig.hasCrsf = false;
        sig.hasRemoteID = true;
        sig.droneClass = 0;  // Consumer
        strlcpy(sig.manufacturer, "DJI", 16);
        sig.active = true;
        db[count++] = sig;
    }
    
    // FPV Racing (5.8 GHz analog video + 2.4 or 900 control)
    {
        DroneSignature sig = {};
        strlcpy(sig.name, "FPV Analog", SIGNATURE_NAME_LEN);
        sig.usesband[RF_BAND_5800] = true;
        sig.usesband[RF_BAND_2400] = true;
        sig.typicalRssiMin = -70;
        sig.typicalRssiMax = -10;
        sig.rssiVarianceThreshold = 15.0f;  // High variance = fast moving
        sig.protocol = 0;
        sig.hasMavlink = false;
        sig.hasCrsf = true;
        sig.hasRemoteID = false;
        sig.droneClass = 1;  // FPV
        strlcpy(sig.manufacturer, "Custom", 16);
        sig.active = true;
        db[count++] = sig;
    }
    
    // ExpressLRS (900 MHz control + 5.8 GHz video)
    {
        DroneSignature sig = {};
        strlcpy(sig.name, "ELRS 900MHz", SIGNATURE_NAME_LEN);
        sig.usesband[RF_BAND_915] = true;
        sig.usesband[RF_BAND_5800] = true;
        sig.typicalRssiMin = -90;
        sig.typicalRssiMax = -30;
        sig.rssiVarianceThreshold = 8.0f;
        sig.protocol = 5;  // PROTOCOL_ELRS
        sig.hasMavlink = false;
        sig.hasCrsf = true;
        sig.hasRemoteID = false;
        sig.hopPatternLength = 4;
        sig.hopChannels[0] = 10;  // Example FHSS channels
        sig.hopChannels[1] = 45;
        sig.hopChannels[2] = 78;
        sig.hopChannels[3] = 112;
        sig.droneClass = 1;  // FPV
        strlcpy(sig.manufacturer, "ELRS", 16);
        sig.active = true;
        db[count++] = sig;
    }
    
    // Crossfire (900 MHz)
    {
        DroneSignature sig = {};
        strlcpy(sig.name, "TBS Crossfire", SIGNATURE_NAME_LEN);
        sig.usesband[RF_BAND_915] = true;
        sig.usesband[RF_BAND_5800] = true;
        sig.typicalRssiMin = -100;
        sig.typicalRssiMax = -20;
        sig.rssiVarianceThreshold = 4.0f;
        sig.protocol = 4;  // PROTOCOL_CROSSFIRE
        sig.hasMavlink = false;
        sig.hasCrsf = true;
        sig.hasRemoteID = false;
        sig.droneClass = 1;  // FPV
        strlcpy(sig.manufacturer, "TBS", 16);
        sig.active = true;
        db[count++] = sig;
    }
    
    // ArduPilot / PX4 (MAVLink on 433/915 MHz telemetry)
    {
        DroneSignature sig = {};
        strlcpy(sig.name, "ArduPilot/PX4", SIGNATURE_NAME_LEN);
        sig.usesband[RF_BAND_433] = true;
        sig.usesband[RF_BAND_915] = true;
        sig.usesband[RF_BAND_2400] = true;
        sig.typicalRssiMin = -85;
        sig.typicalRssiMax = -25;
        sig.rssiVarianceThreshold = 6.0f;
        sig.protocol = 0;
        sig.hasMavlink = true;
        sig.hasCrsf = false;
        sig.hasRemoteID = false;
        sig.droneClass = 2;  // Commercial
        strlcpy(sig.manufacturer, "Open Source", 16);
        sig.active = true;
        db[count++] = sig;
    }
    
    // DJI FPV system (DJI goggles, 5.8 GHz digital)
    {
        DroneSignature sig = {};
        strlcpy(sig.name, "DJI FPV System", SIGNATURE_NAME_LEN);
        sig.usesband[RF_BAND_5800] = true;
        sig.typicalRssiMin = -75;
        sig.typicalRssiMax = -15;
        sig.rssiVarianceThreshold = 12.0f;
        sig.protocol = 2;
        sig.hasMavlink = false;
        sig.hasCrsf = false;
        sig.hasRemoteID = true;
        sig.droneClass = 1;  // FPV
        strlcpy(sig.manufacturer, "DJI", 16);
        sig.active = true;
        db[count++] = sig;
    }
    
    // Autel (2.4 GHz)
    {
        DroneSignature sig = {};
        strlcpy(sig.name, "Autel EVO", SIGNATURE_NAME_LEN);
        sig.usesband[RF_BAND_2400] = true;
        sig.usesband[RF_BAND_5800] = true;
        sig.typicalRssiMin = -78;
        sig.typicalRssiMax = -20;
        sig.rssiVarianceThreshold = 4.0f;
        sig.protocol = 0;
        sig.hasMavlink = false;
        sig.hasCrsf = false;
        sig.hasRemoteID = true;
        sig.droneClass = 0;
        strlcpy(sig.manufacturer, "Autel", 16);
        sig.active = true;
        db[count++] = sig;
    }
    
    // Skydio (2.4 GHz, strong AI-based autonomy)
    {
        DroneSignature sig = {};
        strlcpy(sig.name, "Skydio", SIGNATURE_NAME_LEN);
        sig.usesband[RF_BAND_2400] = true;
        sig.typicalRssiMin = -75;
        sig.typicalRssiMax = -25;
        sig.rssiVarianceThreshold = 3.0f;  // Very stable hover
        sig.protocol = 0;
        sig.hasMavlink = false;
        sig.hasCrsf = false;
        sig.hasRemoteID = true;
        sig.droneClass = 0;
        strlcpy(sig.manufacturer, "Skydio", 16);
        sig.active = true;
        db[count++] = sig;
    }
}

// --- Matching ---

float SignalDatabase::calculateMatchScore(const DroneSignature& sig,
                                            bool bands[5], int8_t rssi, float variance,
                                            bool mavlink, bool crsf, bool remoteID) {
    float score = 0.0f;
    float maxScore = 0.0f;
    
    // Band matching (30% weight)
    maxScore += 30.0f;
    uint8_t bandMatches = 0;
    uint8_t sigBandCount = 0;
    for (uint8_t i = 0; i < 5; i++) {
        if (sig.usesband[i]) sigBandCount++;
        if (sig.usesband[i] && bands[i]) bandMatches++;
    }
    if (sigBandCount > 0) {
        score += 30.0f * ((float)bandMatches / sigBandCount);
    }
    
    // RSSI range match (20% weight)
    maxScore += 20.0f;
    if (rssi >= sig.typicalRssiMin && rssi <= sig.typicalRssiMax) {
        score += 20.0f;
    } else {
        // Partial score for close misses
        int8_t dist = 0;
        if (rssi < sig.typicalRssiMin) dist = sig.typicalRssiMin - rssi;
        if (rssi > sig.typicalRssiMax) dist = rssi - sig.typicalRssiMax;
        if (dist < 20) score += 20.0f * (1.0f - (float)dist / 20.0f);
    }
    
    // Protocol match (30% weight)
    maxScore += 30.0f;
    if (sig.hasMavlink && mavlink) score += 15.0f;
    if (sig.hasCrsf && crsf) score += 15.0f;
    if (!sig.hasMavlink && !sig.hasCrsf && !mavlink && !crsf) score += 10.0f;
    if (sig.hasRemoteID && remoteID) score += 10.0f;
    
    // RSSI variance match (20% weight)
    maxScore += 20.0f;
    float varDiff = fabsf(variance - sig.rssiVarianceThreshold);
    if (varDiff < 5.0f) {
        score += 20.0f * (1.0f - varDiff / 5.0f);
    }
    
    return score / maxScore;  // Normalize to 0-1
}

SignatureMatch SignalDatabase::matchSignal(bool activeBands[5], int8_t rssi, float rssiVariance,
                                            bool mavlink, bool crsf, bool remoteID) {
    SignatureMatch best = {0, 0.0f, "Unknown", 0};
    
    for (uint8_t i = 0; i < count; i++) {
        if (!db[i].active) continue;
        
        float score = calculateMatchScore(db[i], activeBands, rssi, rssiVariance,
                                           mavlink, crsf, remoteID);
        
        if (score > best.confidence) {
            best.signatureIndex = i;
            best.confidence = score;
            best.name = db[i].name;
            best.droneClass = db[i].droneClass;
        }
    }
    
    return best;
}

SignatureMatch SignalDatabase::matchFromHistory(int8_t* rssiHistory, uint16_t histLen,
                                                 bool activeBands[5], bool mavlink, bool crsf) {
    // Calculate average RSSI and variance from history
    float sum = 0, sumSq = 0;
    int8_t maxRssi = -128;
    for (uint16_t i = 0; i < histLen; i++) {
        sum += rssiHistory[i];
        sumSq += (float)rssiHistory[i] * rssiHistory[i];
        if (rssiHistory[i] > maxRssi) maxRssi = rssiHistory[i];
    }
    float avg = sum / histLen;
    float variance = (sumSq / histLen) - (avg * avg);
    
    return matchSignal(activeBands, maxRssi, sqrtf(variance), mavlink, crsf, false);
}

// --- Persistence ---

bool SignalDatabase::addSignature(const DroneSignature& sig) {
    if (count >= MAX_SIGNATURES) return false;
    db[count] = sig;
    db[count].active = true;
    count++;
    return save();
}

bool SignalDatabase::removeSignature(uint8_t index) {
    if (index >= count) return false;
    db[index].active = false;
    return save();
}

const DroneSignature* SignalDatabase::getSignature(uint8_t index) const {
    if (index >= count) return nullptr;
    return &db[index];
}

bool SignalDatabase::save() {
    File file = SPIFFS.open(SIGDB_FILE, "w");
    if (!file) return false;
    
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    
    for (uint8_t i = 0; i < count; i++) {
        if (!db[i].active) continue;
        JsonObject obj = arr.add<JsonObject>();
        obj["name"] = db[i].name;
        obj["mfr"] = db[i].manufacturer;
        obj["class"] = db[i].droneClass;
        obj["rssiMin"] = db[i].typicalRssiMin;
        obj["rssiMax"] = db[i].typicalRssiMax;
        obj["variance"] = db[i].rssiVarianceThreshold;
        obj["mavlink"] = db[i].hasMavlink;
        obj["crsf"] = db[i].hasCrsf;
        obj["remoteID"] = db[i].hasRemoteID;
        
        JsonArray bands = obj["bands"].to<JsonArray>();
        for (uint8_t b = 0; b < 5; b++) {
            bands.add(db[i].usesband[b]);
        }
    }
    
    serializeJson(doc, file);
    file.close();
    return true;
}

bool SignalDatabase::load() {
    if (!SPIFFS.exists(SIGDB_FILE)) return false;
    
    File file = SPIFFS.open(SIGDB_FILE, "r");
    if (!file) return false;
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    
    if (err) return false;
    
    count = 0;
    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        if (count >= MAX_SIGNATURES) break;
        
        DroneSignature& sig = db[count];
        memset(&sig, 0, sizeof(sig));
        
        strlcpy(sig.name, obj["name"] | "Unknown", SIGNATURE_NAME_LEN);
        strlcpy(sig.manufacturer, obj["mfr"] | "", 16);
        sig.droneClass = obj["class"] | 0;
        sig.typicalRssiMin = obj["rssiMin"] | -90;
        sig.typicalRssiMax = obj["rssiMax"] | -20;
        sig.rssiVarianceThreshold = obj["variance"] | 5.0f;
        sig.hasMavlink = obj["mavlink"] | false;
        sig.hasCrsf = obj["crsf"] | false;
        sig.hasRemoteID = obj["remoteID"] | false;
        
        JsonArray bands = obj["bands"];
        for (uint8_t b = 0; b < 5 && b < bands.size(); b++) {
            sig.usesband[b] = bands[b] | false;
        }
        
        sig.active = true;
        count++;
    }
    
    return count > 0;
}

String SignalDatabase::toJSON() const {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    
    for (uint8_t i = 0; i < count; i++) {
        if (!db[i].active) continue;
        JsonObject obj = arr.add<JsonObject>();
        obj["name"] = db[i].name;
        obj["manufacturer"] = db[i].manufacturer;
        obj["class"] = db[i].droneClass;
        
        JsonArray bands = obj["bands"].to<JsonArray>();
        for (uint8_t b = 0; b < 5; b++) {
            if (db[i].usesband[b]) {
                const char* names[] = {"433MHz","868MHz","915MHz","2.4GHz","5.8GHz"};
                bands.add(names[b]);
            }
        }
    }
    
    String out;
    serializeJson(doc, out);
    return out;
}
