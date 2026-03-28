#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <Arduino.h>
#include "config.h"

#ifdef MODULE_GPS

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <vector>

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
    GPSModule(uint8_t rxPin = PIN_GPS_RX, uint8_t txPin = PIN_GPS_TX);
    
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

#endif // MODULE_GPS
#endif // GPS_MODULE_H
