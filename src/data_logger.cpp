#include "data_logger.h"

#ifdef MODULE_SD_CARD

DataLogger::DataLogger() 
    : sdCardAvailable(false), logRotationSize(MAX_LOG_SIZE_MB * 1024 * 1024), 
      currentFileSize(0), minimumLogLevel(LOG_INFO) {
}

bool DataLogger::begin(uint8_t csPin) {
    Serial.printf("[DataLogger] Initializing SD card (CS: GPIO %d)...\n", csPin);
    
    if (!SD.begin(csPin)) {
        Serial.println("[DataLogger] SD card initialization failed");
        sdCardAvailable = false;
        return false;
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("[DataLogger] SD card size: %llu MB\n", cardSize);
    
    if (!createLogDirectory()) {
        Serial.println("[DataLogger] Failed to create log directory");
        return false;
    }
    
    currentLogFile = generateLogFileName();
    sdCardAvailable = true;
    
    Serial.printf("[DataLogger] Logging to: %s\n", currentLogFile.c_str());
    return true;
}

void DataLogger::end() {
    SD.end();
    sdCardAvailable = false;
}

bool DataLogger::createLogDirectory() {
    if (!SD.exists(LOG_DIR)) {
        return SD.mkdir(LOG_DIR);
    }
    return true;
}

String DataLogger::generateLogFileName() {
    char filename[64];
    snprintf(filename, sizeof(filename), "%s/log_%lu.txt", LOG_DIR, millis());
    return String(filename);
}

void DataLogger::rotateLogsIfNeeded() {
    if (currentFileSize >= logRotationSize) {
        currentLogFile = generateLogFileName();
        currentFileSize = 0;
        
        if (getLogFileCount() > MAX_LOG_FILES) {
            deleteOldestLog();
        }
    }
}

void DataLogger::deleteOldestLog() {
    File root = SD.open(LOG_DIR);
    if (!root || !root.isDirectory()) return;
    
    String oldestFile;
    uint32_t oldestTime = UINT32_MAX;
    
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            uint32_t fileTime = file.getLastWrite();
            if (fileTime < oldestTime) {
                oldestTime = fileTime;
                // Build full path: /logs/filename
                oldestFile = String(LOG_DIR) + "/" + String(file.name());
            }
        }
        file.close();  // Close each file handle!
        file = root.openNextFile();
    }
    root.close();
    
    if (oldestFile.length() > 0) {
        SD.remove(oldestFile.c_str());
        Serial.printf("[DataLogger] Deleted old log: %s\n", oldestFile.c_str());
    }
}

uint8_t DataLogger::getLogFileCount() {
    File root = SD.open(LOG_DIR);
    if (!root || !root.isDirectory()) return 0;
    
    uint8_t count = 0;
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) count++;
        file.close();  // Close each file handle!
        file = root.openNextFile();
    }
    root.close();
    return count;
}

bool DataLogger::logDetection(const DetectionLogEntry& entry) {
    if (!sdCardAvailable) return false;
    
    rotateLogsIfNeeded();
    
    File logFile = SD.open(currentLogFile, FILE_APPEND);
    if (!logFile) return false;
    
    char logLine[256];
    snprintf(logLine, sizeof(logLine), 
             "[%lu] DETECTION | Module: %s | RSSI: %d dBm | Freq: %.2f MHz | Protocol: %s | GPS: %.6f,%.6f @ %.1fm\n",
             entry.timestamp, entry.moduleName, entry.rssi, entry.frequency, entry.protocol,
             entry.latitude, entry.longitude, entry.altitude);
    
    size_t written = logFile.print(logLine);
    currentFileSize += written;
    logFile.close();
    
    return written > 0;
}

bool DataLogger::logRFData(const char* module, int rssi, float freq, const char* protocol) {
    DetectionLogEntry entry = {};
    entry.timestamp = millis();
    strncpy(entry.moduleName, module, sizeof(entry.moduleName) - 1);
    entry.rssi = rssi;
    entry.frequency = freq;
    strncpy(entry.protocol, protocol, sizeof(entry.protocol) - 1);
    
    return logDetection(entry);
}

bool DataLogger::logDroneRemoteID(const char* uasID, double lat, double lon, float alt, int rssi) {
    if (!sdCardAvailable) return false;
    
    rotateLogsIfNeeded();
    
    File logFile = SD.open(currentLogFile, FILE_APPEND);
    if (!logFile) return false;
    
    char logLine[256];
    snprintf(logLine, sizeof(logLine),
             "[%lu] REMOTE_ID | UAS: %s | Position: %.6f,%.6f @ %.1fm | RSSI: %d dBm\n",
             millis(), uasID, lat, lon, alt, rssi);
    
    size_t written = logFile.print(logLine);
    currentFileSize += written;
    logFile.close();
    
    return written > 0;
}

bool DataLogger::logSystemEvent(LogLevel level, const char* message) {
    if (!sdCardAvailable || level < minimumLogLevel) return false;
    
    rotateLogsIfNeeded();
    
    File logFile = SD.open(currentLogFile, FILE_APPEND);
    if (!logFile) return false;
    
    const char* levelStr[] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
    
    char logLine[256];
    snprintf(logLine, sizeof(logLine), "[%lu] %s | %s\n", 
             millis(), levelStr[level], message);
    
    size_t written = logFile.print(logLine);
    currentFileSize += written;
    logFile.close();
    
    return written > 0;
}

bool DataLogger::exportToJSON(const char* outputFile, uint32_t startTime, uint32_t endTime) {
    if (!sdCardAvailable) return false;
    
    File output = SD.open(outputFile, FILE_WRITE);
    if (!output) return false;
    
    output.println("{\"detections\":[");
    
    File root = SD.open(LOG_DIR);
    if (!root) { output.close(); return false; }
    
    File file = root.openNextFile();
    bool firstEntry = true;
    
    while (file) {
        if (!file.isDirectory()) {
            while (file.available()) {
                String line = file.readStringUntil('\n');
                if (!firstEntry) output.print(",");
                output.printf("{\"raw\":\"%s\"}", line.c_str());
                firstEntry = false;
            }
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
    
    output.println("]}");
    output.close();
    
    return true;
}

bool DataLogger::exportToCSV(const char* outputFile, uint32_t startTime, uint32_t endTime) {
    if (!sdCardAvailable) return false;
    
    File output = SD.open(outputFile, FILE_WRITE);
    if (!output) return false;
    
    output.println("Timestamp,Type,Module,RSSI,Frequency,Protocol,Latitude,Longitude,Altitude");
    
    File root = SD.open(LOG_DIR);
    if (!root) { output.close(); return false; }
    
    File file = root.openNextFile();
    
    while (file) {
        if (!file.isDirectory()) {
            while (file.available()) {
                String line = file.readStringUntil('\n');
                output.println(line);
            }
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
    
    output.close();
    return true;
}

void DataLogger::setLogLevel(LogLevel level) {
    minimumLogLevel = level;
}

void DataLogger::setRotationSize(uint32_t sizeBytes) {
    logRotationSize = sizeBytes;
}

uint32_t DataLogger::getTotalLogSize() {
    if (!sdCardAvailable) return 0;
    
    uint32_t totalSize = 0;
    File root = SD.open(LOG_DIR);
    if (!root) return 0;
    
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            totalSize += file.size();
        }
        file.close();  // Close each file handle!
        file = root.openNextFile();
    }
    root.close();
    
    return totalSize;
}

bool DataLogger::isSDCardAvailable() const {
    return sdCardAvailable;
}

String DataLogger::getCurrentLogFile() const {
    return currentLogFile;
}

#endif // MODULE_SD_CARD
