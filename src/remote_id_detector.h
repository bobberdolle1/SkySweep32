#ifndef REMOTE_ID_DETECTOR_H
#define REMOTE_ID_DETECTOR_H

#include <Arduino.h>
#include "config.h"

#ifdef MODULE_REMOTE_ID

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ANSI/CTA-2063 Remote ID Standard
#define REMOTE_ID_BLE_SERVICE_UUID "0000fffa-0000-1000-8000-00805f9b34fb"

// Message Types (ASTM F3411-22a)
enum RemoteIDMessageType {
    BASIC_ID = 0x00,
    LOCATION = 0x01,
    AUTH = 0x02,
    SELF_ID = 0x03,
    SYSTEM = 0x04,
    OPERATOR_ID = 0x05
};

struct DroneRemoteIDData {
    char uasID[21];
    uint8_t idType;
    uint8_t uaType;
    
    double latitude;
    double longitude;
    float altitude;
    float height;
    float speed;
    uint16_t direction;
    
    double operatorLatitude;
    double operatorLongitude;
    float operatorAltitude;
    
    float areaRadius;
    uint8_t areaCount;
    uint8_t classification;
    
    int8_t rssi;
    uint32_t lastSeen;
    bool isValid;
    char operatorID[21];
};

class RemoteIDDetector : public BLEAdvertisedDeviceCallbacks {
private:
    BLEScan* bleScan;
    DroneRemoteIDData detectedDrones[MAX_DETECTED_DRONES];  // Fixed-size array instead of vector
    uint8_t droneCount;
    uint32_t scanInterval;
    uint32_t lastScanTime;
    bool isScanning;
    
    void parseBasicIDMessage(const uint8_t* payload, DroneRemoteIDData& drone);
    void parseLocationMessage(const uint8_t* payload, DroneRemoteIDData& drone);
    void parseSystemMessage(const uint8_t* payload, DroneRemoteIDData& drone);
    void parseOperatorIDMessage(const uint8_t* payload, DroneRemoteIDData& drone);
    
    void cleanupOldDetections(uint32_t timeoutMs);
    int findDroneByID(const char* uasID);

public:
    RemoteIDDetector(uint32_t scanIntervalMs = BLE_SCAN_INTERVAL_MS);
    
    bool begin();
    void update();
    void onResult(BLEAdvertisedDevice advertisedDevice) override;
    
    uint8_t getDetectedDroneCount() const;
    DroneRemoteIDData getDroneData(uint8_t index) const;
    void clearDetections();
    
    bool isDroneInRange(double lat, double lon, float radiusMeters);
    void printDroneInfo(uint8_t index);
};

#endif // MODULE_REMOTE_ID
#endif // REMOTE_ID_DETECTOR_H
