#ifndef REMOTE_ID_DETECTOR_H
#define REMOTE_ID_DETECTOR_H

#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ANSI/CTA-2063 Remote ID Standard
#define REMOTE_ID_WIFI_NAN_SERVICE "org.opendroneid.remoteid"
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
    char uasID[21];                  // UAS Serial Number
    uint8_t idType;                  // 0=None, 1=Serial, 2=CAA, 3=UTM
    uint8_t uaType;                  // 0=None, 1=Aeroplane, 2=Helicopter, 3=Multirotor, etc.
    
    // Location data
    double latitude;
    double longitude;
    float altitude;                  // Meters (WGS84)
    float height;                    // Meters AGL
    float speed;                     // m/s
    uint16_t direction;              // Degrees (0-360)
    
    // Operator location
    double operatorLatitude;
    double operatorLongitude;
    float operatorAltitude;
    
    // System data
    float areaRadius;                // Meters
    uint8_t areaCount;
    uint8_t classification;
    
    // Metadata
    int8_t rssi;
    uint32_t lastSeen;
    bool isValid;
    char operatorID[21];
};

class RemoteIDDetector : public BLEAdvertisedDeviceCallbacks {
private:
    BLEScan* bleScan;
    std::vector<DroneRemoteIDData> detectedDrones;
    uint32_t scanInterval;
    bool isScanning;
    
    void parseBasicIDMessage(const uint8_t* payload, DroneRemoteIDData& drone);
    void parseLocationMessage(const uint8_t* payload, DroneRemoteIDData& drone);
    void parseSystemMessage(const uint8_t* payload, DroneRemoteIDData& drone);
    void parseOperatorIDMessage(const uint8_t* payload, DroneRemoteIDData& drone);
    
    void scanWiFiBeacons();
    void cleanupOldDetections(uint32_t timeoutMs);

public:
    RemoteIDDetector(uint32_t scanIntervalMs = 5000);
    
    bool begin();
    void update();
    void onResult(BLEAdvertisedDevice advertisedDevice) override;
    
    uint8_t getDetectedDroneCount() const;
    DroneRemoteIDData getDroneData(uint8_t index) const;
    void clearDetections();
    
    bool isDroneInRange(double lat, double lon, float radiusMeters);
    void printDroneInfo(uint8_t index);
};

#endif
