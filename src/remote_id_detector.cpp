#include "remote_id_detector.h"
#include <cmath>

RemoteIDDetector::RemoteIDDetector(uint32_t scanIntervalMs) 
    : scanInterval(scanIntervalMs), isScanning(false), bleScan(nullptr) {
}

bool RemoteIDDetector::begin() {
    Serial.println("[RemoteID] Initializing BLE scanner...");
    
    BLEDevice::init("SkySweep32_Scanner");
    bleScan = BLEDevice::getScan();
    bleScan->setAdvertisedDeviceCallbacks(this);
    bleScan->setActiveScan(true);
    bleScan->setInterval(100);
    bleScan->setWindow(99);
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    Serial.println("[RemoteID] BLE and WiFi scanners ready");
    return true;
}

void RemoteIDDetector::update() {
    cleanupOldDetections(30000); // Remove detections older than 30s
    
    if (!isScanning) {
        isScanning = true;
        bleScan->start(scanInterval / 1000, false);
        scanWiFiBeacons();
        isScanning = false;
    }
}

void RemoteIDDetector::onResult(BLEAdvertisedDevice advertisedDevice) {
    if (!advertisedDevice.haveServiceUUID()) return;
    
    BLEUUID serviceUUID = advertisedDevice.getServiceUUID();
    if (serviceUUID.toString() != REMOTE_ID_BLE_SERVICE_UUID) return;
    
    if (!advertisedDevice.haveServiceData()) return;
    
    std::string serviceData = advertisedDevice.getServiceData();
    const uint8_t* payload = (const uint8_t*)serviceData.c_str();
    uint8_t payloadLength = serviceData.length();
    
    if (payloadLength < 2) return;
    
    uint8_t messageType = payload[0];
    DroneRemoteIDData drone = {};
    drone.rssi = advertisedDevice.getRSSI();
    drone.lastSeen = millis();
    drone.isValid = false;
    
    switch (messageType) {
        case BASIC_ID:
            parseBasicIDMessage(payload, drone);
            break;
        case LOCATION:
            parseLocationMessage(payload, drone);
            break;
        case SYSTEM:
            parseSystemMessage(payload, drone);
            break;
        case OPERATOR_ID:
            parseOperatorIDMessage(payload, drone);
            break;
        default:
            return;
    }
    
    // Update existing or add new
    bool found = false;
    for (auto& existing : detectedDrones) {
        if (strcmp(existing.uasID, drone.uasID) == 0) {
            if (drone.latitude != 0.0) existing.latitude = drone.latitude;
            if (drone.longitude != 0.0) existing.longitude = drone.longitude;
            if (drone.altitude != 0.0) existing.altitude = drone.altitude;
            existing.rssi = drone.rssi;
            existing.lastSeen = drone.lastSeen;
            found = true;
            break;
        }
    }
    
    if (!found && drone.isValid) {
        detectedDrones.push_back(drone);
        Serial.printf("[RemoteID] New drone detected: %s (RSSI: %d dBm)\n", 
                     drone.uasID, drone.rssi);
    }
}

void RemoteIDDetector::parseBasicIDMessage(const uint8_t* payload, DroneRemoteIDData& drone) {
    if (payload[0] != BASIC_ID) return;
    
    drone.idType = payload[1];
    drone.uaType = payload[2];
    
    memcpy(drone.uasID, &payload[3], 20);
    drone.uasID[20] = '\0';
    
    drone.isValid = (drone.idType > 0 && strlen(drone.uasID) > 0);
}

void RemoteIDDetector::parseLocationMessage(const uint8_t* payload, DroneRemoteIDData& drone) {
    if (payload[0] != LOCATION) return;
    
    // ASTM F3411-22a Location Message Format
    int32_t latRaw = (payload[2] << 24) | (payload[3] << 16) | (payload[4] << 8) | payload[5];
    int32_t lonRaw = (payload[6] << 24) | (payload[7] << 16) | (payload[8] << 8) | payload[9];
    
    drone.latitude = latRaw / 1e7;
    drone.longitude = lonRaw / 1e7;
    
    uint16_t altRaw = (payload[10] << 8) | payload[11];
    drone.altitude = (altRaw - 1000) * 0.5f;
    
    uint16_t heightRaw = (payload[12] << 8) | payload[13];
    drone.height = (heightRaw - 1000) * 0.5f;
    
    drone.speed = payload[14] * 0.25f;
    drone.direction = ((payload[15] << 8) | payload[16]) / 100;
    
    drone.isValid = (drone.latitude != 0.0 && drone.longitude != 0.0);
}

void RemoteIDDetector::parseSystemMessage(const uint8_t* payload, DroneRemoteIDData& drone) {
    if (payload[0] != SYSTEM) return;
    
    int32_t opLatRaw = (payload[2] << 24) | (payload[3] << 16) | (payload[4] << 8) | payload[5];
    int32_t opLonRaw = (payload[6] << 24) | (payload[7] << 16) | (payload[8] << 8) | payload[9];
    
    drone.operatorLatitude = opLatRaw / 1e7;
    drone.operatorLongitude = opLonRaw / 1e7;
    
    drone.areaRadius = ((payload[10] << 8) | payload[11]) * 10.0f;
    drone.classification = payload[12];
    
    drone.isValid = true;
}

void RemoteIDDetector::parseOperatorIDMessage(const uint8_t* payload, DroneRemoteIDData& drone) {
    if (payload[0] != OPERATOR_ID) return;
    
    memcpy(drone.operatorID, &payload[2], 20);
    drone.operatorID[20] = '\0';
    
    drone.isValid = true;
}

void RemoteIDDetector::scanWiFiBeacons() {
    int networkCount = WiFi.scanNetworks(false, false, false, 300);
    
    for (int i = 0; i < networkCount; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.indexOf("DJI") >= 0 || ssid.indexOf("DRONE") >= 0) {
            Serial.printf("[RemoteID] WiFi beacon detected: %s (RSSI: %d dBm)\n", 
                         ssid.c_str(), WiFi.RSSI(i));
        }
    }
    
    WiFi.scanDelete();
}

void RemoteIDDetector::cleanupOldDetections(uint32_t timeoutMs) {
    uint32_t currentTime = millis();
    detectedDrones.erase(
        std::remove_if(detectedDrones.begin(), detectedDrones.end(),
            [currentTime, timeoutMs](const DroneRemoteIDData& d) {
                return (currentTime - d.lastSeen) > timeoutMs;
            }),
        detectedDrones.end()
    );
}

uint8_t RemoteIDDetector::getDetectedDroneCount() const {
    return detectedDrones.size();
}

DroneRemoteIDData RemoteIDDetector::getDroneData(uint8_t index) const {
    if (index < detectedDrones.size()) {
        return detectedDrones[index];
    }
    return DroneRemoteIDData{};
}

void RemoteIDDetector::clearDetections() {
    detectedDrones.clear();
}

bool RemoteIDDetector::isDroneInRange(double lat, double lon, float radiusMeters) {
    for (const auto& drone : detectedDrones) {
        double dLat = (drone.latitude - lat) * 111320.0;
        double dLon = (drone.longitude - lon) * 111320.0 * cos(lat * M_PI / 180.0);
        double distance = sqrt(dLat * dLat + dLon * dLon);
        
        if (distance <= radiusMeters) return true;
    }
    return false;
}

void RemoteIDDetector::printDroneInfo(uint8_t index) {
    if (index >= detectedDrones.size()) return;
    
    const auto& drone = detectedDrones[index];
    Serial.println("=== Remote ID Drone Info ===");
    Serial.printf("UAS ID: %s\n", drone.uasID);
    Serial.printf("Type: %d\n", drone.uaType);
    Serial.printf("Position: %.6f, %.6f @ %.1fm\n", 
                 drone.latitude, drone.longitude, drone.altitude);
    Serial.printf("Speed: %.1f m/s, Direction: %d°\n", drone.speed, drone.direction);
    Serial.printf("Operator: %.6f, %.6f\n", 
                 drone.operatorLatitude, drone.operatorLongitude);
    Serial.printf("RSSI: %d dBm, Age: %lu ms\n", 
                 drone.rssi, millis() - drone.lastSeen);
}
