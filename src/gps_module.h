#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

#define GPS_BAUD_RATE 9600
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_UPDATE_INTERVAL 1000

struct GPSData {
    double latitude;
    double longitude;
    float altitude;
    float speed;
    uint16_t course;
    uint8_t satellites;
    float hdop;
    bool isValid;
    uint32_t lastUpdate;
};

struct GeofenceZone {
    double centerLat;
    double centerLon;
    float radiusMeters;
    bool alertOnEntry;
    bool isActive;
    char name[32];
};

class GPSModule {
private:
    TinyGPSPlus gps;
    HardwareSerial* gpsSerial;
    GPSData currentData;
    uint32_t lastUpdateTime;
    
    std::vector<GeofenceZone> geofences;
    
    float calculateDistance(double lat1, double lon1, double lat2, double lon2);
    bool isInsideGeofence(const GeofenceZone& zone, double lat, double lon);

public:
    GPSModule(uint8_t rxPin = GPS_RX_PIN, uint8_t txPin = GPS_TX_PIN);
    
    bool begin(uint32_t baudRate = GPS_BAUD_RATE);
    void update();
    
    GPSData getData() const;
    bool isFixValid() const;
    
    void addGeofence(const char* name, double lat, double lon, float radius, bool alertOnEntry = true);
    void removeGeofence(const char* name);
    void clearGeofences();
    bool checkGeofenceViolations(double lat, double lon);
    
    float getDistanceToPoint(double targetLat, double targetLon);
    uint16_t getBearingToPoint(double targetLat, double targetLon);
    
    void printInfo();
};

#endif
