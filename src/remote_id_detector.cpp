#include "remote_id_detector.h"

#ifdef MODULE_REMOTE_ID

#include <cmath>

RemoteIDDetector::RemoteIDDetector(uint32_t scanIntervalMs) 
    : scanInterval(scanIntervalMs), isScanning(false), bleScan(nullptr), 
      droneCount(0), lastScanTime(0) {
    memset(detectedDrones, 0, sizeof(detectedDrones));
}

bool RemoteIDDetector::begin() {
    Serial.println("[RemoteID] Initializing BLE scanner...");
    
    BLEDevice::init("SkySweep32");
    bleScan = BLEDevice::getScan();
    bleScan->setAdvertisedDeviceCallbacks(this);
    bleScan->setActiveScan(true);
    bleScan->setInterval(100);
    bleScan->setWindow(99);
    
    // NOTE: WiFi mode is managed by web_server.cpp (WIFI_AP_STA)
    // Do NOT call WiFi.mode() or WiFi.disconnect() here!
    
    Serial.println("[RemoteID] BLE scanner ready");
    return true;
}

void RemoteIDDetector::update() {
    // Only scan periodically (non-blocking)
    if (millis() - lastScanTime < scanInterval) return;
    lastScanTime = millis();
    
    cleanupOldDetections(REMOTE_ID_CLEANUP_MS);
    
    if (!isScanning) {
        isScanning = true;
        // Non-blocking BLE scan (duration in seconds)
        bleScan->start(scanInterval / 1000, false);
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
    
    // Update existing drone or add new one
    int existingIdx = findDroneByID(drone.uasID);
    
    if (existingIdx >= 0) {
        // Update existing
        if (drone.latitude != 0.0) detectedDrones[existingIdx].latitude = drone.latitude;
        if (drone.longitude != 0.0) detectedDrones[existingIdx].longitude = drone.longitude;
        if (drone.altitude != 0.0) detectedDrones[existingIdx].altitude = drone.altitude;
        detectedDrones[existingIdx].rssi = drone.rssi;
        detectedDrones[existingIdx].lastSeen = drone.lastSeen;
    } else if (drone.isValid && droneCount < MAX_DETECTED_DRONES) {
        // Add new (with bounds check!)
        detectedDrones[droneCount] = drone;
        droneCount++;
        Serial.printf("[RemoteID] New drone detected: %s (RSSI: %d dBm) [%d/%d]\n", 
                     drone.uasID, drone.rssi, droneCount, MAX_DETECTED_DRONES);
    }
}

int RemoteIDDetector::findDroneByID(const char* uasID) {
    for (uint8_t i = 0; i < droneCount; i++) {
        if (strcmp(detectedDrones[i].uasID, uasID) == 0) {
            return i;
        }
    }
    return -1;
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

void RemoteIDDetector::cleanupOldDetections(uint32_t timeoutMs) {
    uint32_t currentTime = millis();
    uint8_t writeIdx = 0;
    
    for (uint8_t i = 0; i < droneCount; i++) {
        if ((currentTime - detectedDrones[i].lastSeen) <= timeoutMs) {
            if (writeIdx != i) {
                detectedDrones[writeIdx] = detectedDrones[i];
            }
            writeIdx++;
        }
    }
    
    droneCount = writeIdx;
}

uint8_t RemoteIDDetector::getDetectedDroneCount() const {
    return droneCount;
}

DroneRemoteIDData RemoteIDDetector::getDroneData(uint8_t index) const {
    if (index < droneCount) {
        return detectedDrones[index];
    }
    return DroneRemoteIDData{};
}

void RemoteIDDetector::clearDetections() {
    droneCount = 0;
    memset(detectedDrones, 0, sizeof(detectedDrones));
}

bool RemoteIDDetector::isDroneInRange(double lat, double lon, float radiusMeters) {
    for (uint8_t i = 0; i < droneCount; i++) {
        double dLat = (detectedDrones[i].latitude - lat) * 111320.0;
        double dLon = (detectedDrones[i].longitude - lon) * 111320.0 * cos(lat * M_PI / 180.0);
        double distance = sqrt(dLat * dLat + dLon * dLon);
        
        if (distance <= radiusMeters) return true;
    }
    return false;
}

void RemoteIDDetector::printDroneInfo(uint8_t index) {
    if (index >= droneCount) return;
    
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

#endif // MODULE_REMOTE_ID
