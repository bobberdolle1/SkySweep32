#include "gps_module.h"
#include <cmath>

GPSModule::GPSModule(uint8_t rxPin, uint8_t txPin) : lastUpdateTime(0) {
    gpsSerial = new HardwareSerial(2);
    currentData = {};
}

bool GPSModule::begin(uint32_t baudRate) {
    Serial.println("[GPS] Initializing GPS module...");
    
    gpsSerial->begin(baudRate, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    
    delay(1000);
    
    Serial.println("[GPS] GPS module ready");
    return true;
}

void GPSModule::update() {
    while (gpsSerial->available() > 0) {
        char c = gpsSerial->read();
        gps.encode(c);
    }
    
    if (millis() - lastUpdateTime >= GPS_UPDATE_INTERVAL) {
        if (gps.location.isValid()) {
            currentData.latitude = gps.location.lat();
            currentData.longitude = gps.location.lng();
            currentData.altitude = gps.altitude.meters();
            currentData.speed = gps.speed.mps();
            currentData.course = gps.course.deg();
            currentData.satellites = gps.satellites.value();
            currentData.hdop = gps.hdop.hdop();
            currentData.isValid = true;
            currentData.lastUpdate = millis();
            
            checkGeofenceViolations(currentData.latitude, currentData.longitude);
        } else {
            currentData.isValid = false;
        }
        
        lastUpdateTime = millis();
    }
}

GPSData GPSModule::getData() const {
    return currentData;
}

bool GPSModule::isFixValid() const {
    return currentData.isValid && (millis() - currentData.lastUpdate) < 5000;
}

void GPSModule::addGeofence(const char* name, double lat, double lon, float radius, bool alertOnEntry) {
    GeofenceZone zone;
    zone.centerLat = lat;
    zone.centerLon = lon;
    zone.radiusMeters = radius;
    zone.alertOnEntry = alertOnEntry;
    zone.isActive = true;
    strncpy(zone.name, name, sizeof(zone.name) - 1);
    
    geofences.push_back(zone);
    Serial.printf("[GPS] Geofence added: %s (%.6f, %.6f, %.0fm)\n", name, lat, lon, radius);
}

void GPSModule::removeGeofence(const char* name) {
    geofences.erase(
        std::remove_if(geofences.begin(), geofences.end(),
            [name](const GeofenceZone& z) { return strcmp(z.name, name) == 0; }),
        geofences.end()
    );
}

void GPSModule::clearGeofences() {
    geofences.clear();
}

bool GPSModule::checkGeofenceViolations(double lat, double lon) {
    bool violation = false;
    
    for (const auto& zone : geofences) {
        if (!zone.isActive) continue;
        
        bool inside = isInsideGeofence(zone, lat, lon);
        
        if ((zone.alertOnEntry && inside) || (!zone.alertOnEntry && !inside)) {
            Serial.printf("[GPS] GEOFENCE ALERT: %s (%.6f, %.6f)\n", zone.name, lat, lon);
            violation = true;
        }
    }
    
    return violation;
}

bool GPSModule::isInsideGeofence(const GeofenceZone& zone, double lat, double lon) {
    float distance = calculateDistance(zone.centerLat, zone.centerLon, lat, lon);
    return distance <= zone.radiusMeters;
}

float GPSModule::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const float R = 6371000.0f;
    
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    
    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dLon / 2) * sin(dLon / 2);
    
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    
    return R * c;
}

float GPSModule::getDistanceToPoint(double targetLat, double targetLon) {
    if (!currentData.isValid) return -1.0f;
    return calculateDistance(currentData.latitude, currentData.longitude, targetLat, targetLon);
}

uint16_t GPSModule::getBearingToPoint(double targetLat, double targetLon) {
    if (!currentData.isValid) return 0;
    
    double dLon = (targetLon - currentData.longitude) * M_PI / 180.0;
    double lat1 = currentData.latitude * M_PI / 180.0;
    double lat2 = targetLat * M_PI / 180.0;
    
    double y = sin(dLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
    
    double bearing = atan2(y, x) * 180.0 / M_PI;
    return (uint16_t)((int)(bearing + 360) % 360);
}

void GPSModule::printInfo() {
    Serial.println("=== GPS Module Info ===");
    Serial.printf("Position: %.6f, %.6f\n", currentData.latitude, currentData.longitude);
    Serial.printf("Altitude: %.1f m\n", currentData.altitude);
    Serial.printf("Speed: %.1f m/s\n", currentData.speed);
    Serial.printf("Course: %d°\n", currentData.course);
    Serial.printf("Satellites: %d\n", currentData.satellites);
    Serial.printf("HDOP: %.2f\n", currentData.hdop);
    Serial.printf("Valid: %s\n", currentData.isValid ? "YES" : "NO");
}
