#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <Arduino.h>
#include "config.h"

#ifdef MODULE_SD_CARD

#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>

enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3,
    LOG_CRITICAL = 4
};

struct DetectionLogEntry {
    uint32_t timestamp;
    char moduleName[16];
    int rssi;
    float frequency;
    char protocol[16];
    double latitude;
    double longitude;
    float altitude;
};

class DataLogger {
private:
    bool sdCardAvailable;
    String currentLogFile;
    uint32_t logRotationSize;
    uint32_t currentFileSize;
    LogLevel minimumLogLevel;
    
    bool createLogDirectory();
    String generateLogFileName();
    void rotateLogsIfNeeded();
    void deleteOldestLog();
    uint8_t getLogFileCount();

public:
    DataLogger();
    
    bool begin(uint8_t csPin = PIN_SD_CS);
    void end();
    
    bool logDetection(const DetectionLogEntry& entry);
    bool logRFData(const char* module, int rssi, float freq, const char* protocol);
    bool logDroneRemoteID(const char* uasID, double lat, double lon, float alt, int rssi);
    bool logSystemEvent(LogLevel level, const char* message);
    
    bool exportToJSON(const char* outputFile, uint32_t startTime, uint32_t endTime);
    bool exportToCSV(const char* outputFile, uint32_t startTime, uint32_t endTime);
    
    void setLogLevel(LogLevel level);
    void setRotationSize(uint32_t sizeBytes);
    
    uint32_t getTotalLogSize();
    bool isSDCardAvailable() const;
    String getCurrentLogFile() const;
};

#endif // MODULE_SD_CARD
#endif // DATA_LOGGER_H
