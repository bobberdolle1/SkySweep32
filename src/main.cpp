#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "countermeasures.h"
#include "drivers/cc1101.h"
#include "drivers/nrf24l01.h"
#include "drivers/rx5808.h"
#include "protocols/mavlink_parser.h"
#include "protocols/crsf_parser.h"
#include "acoustic_detector.h"
#include "remote_id_detector.h"
#include "web_server.h"
#include "data_logger.h"
#include "gps_module.h"
#include "meshtastic_client.h"
#include "ml_classifier.h"

// SPI Pin Definitions (ESP32 Default)
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK  18

// Chip Select Pins for SPI Devices
#define CS_CC1101    5   // 900 MHz module
#define CS_NRF24L01  17  // 2.4 GHz module
#define CS_RX5808    16  // 5.8 GHz module

// I2C Pin Definitions (ESP32 Default)
#define I2C_SDA 21
#define I2C_SCL 22

// RX5808 RSSI Analog Input
#define RX5808_RSSI_PIN 34

// OLED Display Instance (128x64, I2C)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);

// RF Module Driver Instances
CC1101Driver cc1101(CS_CC1101);
NRF24L01Driver nrf24(CS_NRF24L01, 4); // CE pin = GPIO 4
RX5808Driver rx5808(CS_RX5808, RX5808_RSSI_PIN);

// Protocol Parsers
MAVLinkParser mavlinkParser;
CRSFParser crsfParser;

// Acoustic Detector (optional - requires MEMS microphone)
// Uncomment if ICS-43434 or similar I2S microphone is connected
// #define ENABLE_ACOUSTIC_DETECTION
#ifdef ENABLE_ACOUSTIC_DETECTION
AcousticDetector acousticDetector(43, 44, 1); // BCLK, WS, DIN pins
#endif

// RSSI Storage
struct RFModuleData {
    const char* moduleName;
    uint8_t chipSelectPin;
    int rssiValue;
    bool isActive;
};

RFModuleData rfModules[3] = {
    {"CC1101",    CS_CC1101,    0, false},
    {"NRF24L01+", CS_NRF24L01,  0, false},
    {"RX5808",    CS_RX5808,    0, false}
};

uint8_t currentModuleIndex = 0;

// Countermeasure system instance
CountermeasureSystem counterMeasures;

// New feature modules
RemoteIDDetector remoteIDDetector(5000);
SkySweepWebServer webServer;
DataLogger dataLogger;
GPSModule gpsModule;
MeshtasticClient meshtasticClient;
MLClassifier mlClassifier;

// Feature flags
#define ENABLE_REMOTE_ID
#define ENABLE_WEB_SERVER
#define ENABLE_DATA_LOGGING
#define ENABLE_GPS
#define ENABLE_MESHTASTIC
#define ENABLE_ML_CLASSIFICATION

void initializeSPI() {
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(1000000); // 1 MHz for safe initialization
    
    // Configure all CS pins as OUTPUT and set HIGH (deselected)
    pinMode(CS_CC1101, OUTPUT);
    pinMode(CS_NRF24L01, OUTPUT);
    pinMode(CS_RX5808, OUTPUT);
    
    digitalWrite(CS_CC1101, HIGH);
    digitalWrite(CS_NRF24L01, HIGH);
    digitalWrite(CS_RX5808, HIGH);
    
    Serial.println("[SPI] Initialized - MOSI:23, MISO:19, SCK:18");
}

void initializeI2C() {
    Wire.begin(I2C_SDA, I2C_SCL);
    display.begin();
    display.setFont(u8g2_font_6x10_tr);
    display.clearBuffer();
    display.drawStr(0, 10, "SkySweep32");
    display.drawStr(0, 25, "Initializing...");
    display.sendBuffer();
    Serial.println("[I2C] Initialized - SDA:21, SCL:22");
}

void selectSPIDevice(uint8_t chipSelectPin) {
    // Deselect all devices first
    digitalWrite(CS_CC1101, HIGH);
    digitalWrite(CS_NRF24L01, HIGH);
    digitalWrite(CS_RX5808, HIGH);
    
    // Select target device
    digitalWrite(chipSelectPin, LOW);
    delayMicroseconds(10);
}

void deselectAllSPIDevices() {
    digitalWrite(CS_CC1101, HIGH);
    digitalWrite(CS_NRF24L01, HIGH);
    digitalWrite(CS_RX5808, HIGH);
}

int readModuleRSSI(uint8_t moduleIndex) {
    int rssiValue = 0;
    
    switch(moduleIndex) {
        case 0: // CC1101
            rssiValue = cc1101.readRSSI();
            rssiValue = map(rssiValue, -120, -30, 0, 100); // Convert dBm to percentage
            break;
            
        case 1: // NRF24L01+
            rssiValue = nrf24.readRSSI();
            rssiValue = map(rssiValue, -90, -40, 0, 100); // Convert dBm to percentage
            break;
            
        case 2: // RX5808
            rssiValue = rx5808.readRSSI(); // Already returns percentage
            break;
    }
    
    return constrain(rssiValue, 0, 100);
}

void updateDisplay() {
    display.clearBuffer();
    
    // Header
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 10, "SkySweep32 Monitor");
    display.drawLine(0, 12, 128, 12);
    
    // Module Data
    display.setFont(u8g2_font_5x7_tr);
    int yPosition = 25;
    
    for (int i = 0; i < 3; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%s: %d dBm", 
                 rfModules[i].moduleName, 
                 rfModules[i].rssiValue);
        display.drawStr(0, yPosition, buffer);
        
        // Draw signal strength bar
        int barWidth = map(rfModules[i].rssiValue, 0, 100, 0, 60);
        display.drawFrame(65, yPosition - 7, 62, 8);
        display.drawBox(66, yPosition - 6, barWidth, 6);
        
        yPosition += 12;
    }
    
    // Threat status indicator
    ThreatData threat = counterMeasures.getCurrentThreat();
    display.setFont(u8g2_font_5x7_tr);
    if (threat.isActive) {
        char threatBuffer[32];
        snprintf(threatBuffer, sizeof(threatBuffer), "THREAT: %s", 
                 counterMeasures.getThreatLevelString(threat.level));
        display.drawStr(0, 63, threatBuffer);
    } else {
        display.drawStr(0, 63, counterMeasures.isArmed() ? "ARMED - Scanning" : "Scanning...");
    }
    
    display.sendBuffer();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== SkySweep32 Passive Drone Detector ===");
    Serial.println("Multi-band RF Scanner: 900MHz | 2.4GHz | 5.8GHz\n");
    
    initializeSPI();
    initializeI2C();
    
    pinMode(RX5808_RSSI_PIN, INPUT);
    pinMode(4, OUTPUT); // NRF24 CE pin
    
    // Initialize RF modules
    Serial.println("\n[INIT] Initializing RF modules...");
    if (!cc1101.begin()) {
        Serial.println("[ERROR] CC1101 initialization failed");
    }
    if (!nrf24.begin()) {
        Serial.println("[ERROR] NRF24L01+ initialization failed");
    }
    if (!rx5808.begin()) {
        Serial.println("[ERROR] RX5808 initialization failed");
    }
    
    // Initialize countermeasure system
    counterMeasures.initialize();
    // counterMeasures.armSystem(true);  // Uncomment to enable auto-response
    
    #ifdef ENABLE_ACOUSTIC_DETECTION
    Serial.println("\n[INIT] Initializing acoustic detector...");
    if (!acousticDetector.begin()) {
        Serial.println("[ERROR] Acoustic detector initialization failed");
    }
    // acousticDetector.calibrate(10000); // Uncomment for 10s calibration
    #endif
    
    #ifdef ENABLE_REMOTE_ID
    Serial.println("\n[INIT] Initializing Remote ID detector...");
    if (!remoteIDDetector.begin()) {
        Serial.println("[ERROR] Remote ID detector initialization failed");
    }
    #endif
    
    #ifdef ENABLE_WEB_SERVER
    Serial.println("\n[INIT] Starting web server...");
    if (!webServer.begin(true)) {
        Serial.println("[ERROR] Web server initialization failed");
    } else {
        Serial.printf("[WEB] Dashboard: http://%s\n", webServer.getIPAddress().toString().c_str());
    }
    #endif
    
    #ifdef ENABLE_DATA_LOGGING
    Serial.println("\n[INIT] Initializing SD card logger...");
    if (!dataLogger.begin(15)) {
        Serial.println("[ERROR] Data logger initialization failed");
    }
    #endif
    
    #ifdef ENABLE_GPS
    Serial.println("\n[INIT] Initializing GPS module...");
    if (!gpsModule.begin()) {
        Serial.println("[ERROR] GPS module initialization failed");
    }
    gpsModule.addGeofence("Protected Zone", 0.0, 0.0, 1000.0, true);
    #endif
    
    #ifdef ENABLE_MESHTASTIC
    Serial.println("\n[INIT] Initializing Meshtastic LoRa...");
    if (!meshtasticClient.begin(915.0)) {
        Serial.println("[ERROR] Meshtastic initialization failed");
    }
    #endif
    
    #ifdef ENABLE_ML_CLASSIFICATION
    Serial.println("\n[INIT] Initializing ML classifier...");
    if (!mlClassifier.begin()) {
        Serial.println("[ERROR] ML classifier initialization failed");
    }
    #endif
    
    Serial.println("[SYSTEM] Initialization complete");
    Serial.println("[SYSTEM] Starting round-robin RSSI polling...\n");
    
    delay(2000);
}

void loop() {
    // Acoustic detection (if enabled)
    #ifdef ENABLE_ACOUSTIC_DETECTION
    acousticDetector.update();
    if (acousticDetector.isDroneDetected()) {
        Serial.printf("[ACOUSTIC] Drone detected! Ratio: %.6f\n", 
                     acousticDetector.getCurrentRatio());
    }
    #endif
    
    // Remote ID scanning
    #ifdef ENABLE_REMOTE_ID
    remoteIDDetector.update();
    uint8_t droneCount = remoteIDDetector.getDetectedDroneCount();
    for (uint8_t i = 0; i < droneCount; i++) {
        DroneRemoteIDData drone = remoteIDDetector.getDroneData(i);
        if (drone.isValid) {
            #ifdef ENABLE_WEB_SERVER
            webServer.broadcastDroneDetection(drone.uasID, drone.latitude, drone.longitude, drone.altitude);
            #endif
            #ifdef ENABLE_DATA_LOGGING
            dataLogger.logDroneRemoteID(drone.uasID, drone.latitude, drone.longitude, drone.altitude, drone.rssi);
            #endif
        }
    }
    #endif
    
    // GPS update
    #ifdef ENABLE_GPS
    gpsModule.update();
    if (gpsModule.isFixValid()) {
        GPSData gpsData = gpsModule.getData();
    }
    #endif
    
    // Meshtastic mesh networking
    #ifdef ENABLE_MESHTASTIC
    meshtasticClient.update();
    #endif
    
    // Web server update
    #ifdef ENABLE_WEB_SERVER
    webServer.update();
    #endif
    
    // Round-robin polling of all RF modules
    static int rssiHistory[32] = {0};
    static uint8_t rssiIndex = 0;
    
    for (uint8_t i = 0; i < 3; i++) {
        currentModuleIndex = i;
        
        // Read RSSI from current module
        rfModules[i].rssiValue = readModuleRSSI(i);
        rfModules[i].isActive = (rfModules[i].rssiValue > 40);
        
        // Store RSSI history for ML
        rssiHistory[rssiIndex] = rfModules[i].rssiValue;
        rssiIndex = (rssiIndex + 1) % 32;
        
        // Threat assessment and auto-response
        ThreatLevel threat = counterMeasures.assessThreat(i, rfModules[i].rssiValue);
        
        // Serial output with threat level
        Serial.printf("[%s] RSSI: %d dBm | Threat: %s %s\n", 
                      rfModules[i].moduleName,
                      rfModules[i].rssiValue,
                      counterMeasures.getThreatLevelString(threat),
                      rfModules[i].isActive ? "*** SIGNAL DETECTED ***" : "");
        
        // ML classification on high threat
        #ifdef ENABLE_ML_CLASSIFICATION
        if (threat >= THREAT_HIGH && mlClassifier.isModelLoaded()) {
            ClassificationResult mlResult = mlClassifier.classifyFromRSSI(rssiHistory, 32);
            if (mlResult.isValid) {
                Serial.printf("[ML] Classified as: %s (%.1f%% confidence)\n", 
                             mlClassifier.getClassName(mlResult.droneClass), 
                             mlResult.confidence * 100.0f);
            }
        }
        #endif
        
        // Broadcast to web clients
        #ifdef ENABLE_WEB_SERVER
        webServer.broadcastRFData(rfModules[i].moduleName, rfModules[i].rssiValue, rfModules[i].isActive);
        #endif
        
        // Log detection
        #ifdef ENABLE_DATA_LOGGING
        if (rfModules[i].isActive) {
            float freq = (i == 0) ? 915.0f : (i == 1) ? 2400.0f : 5800.0f;
            dataLogger.logRFData(rfModules[i].moduleName, rfModules[i].rssiValue, freq, "Unknown");
        }
        #endif
        
        // Meshtastic alert broadcast
        #ifdef ENABLE_MESHTASTIC
        if (threat >= THREAT_CRITICAL) {
            DetectionAlert alert = {};
            #ifdef ENABLE_GPS
            GPSData gpsData = gpsModule.getData();
            alert.latitude = gpsData.latitude;
            alert.longitude = gpsData.longitude;
            alert.altitude = gpsData.altitude;
            #endif
            alert.rssi = rfModules[i].rssiValue;
            alert.threatLevel = (uint8_t)threat;
            alert.timestamp = millis();
            snprintf(alert.droneID, sizeof(alert.droneID), "%s_THREAT", rfModules[i].moduleName);
            
            meshtasticClient.broadcastDetectionAlert(alert);
        }
        #endif
        
        // Execute countermeasures if armed and threat detected
        if (counterMeasures.isArmed() && threat >= THREAT_HIGH) {
            counterMeasures.autoRespond(i, rfModules[i].rssiValue, rfModules[i].chipSelectPin);
        }
        
        delay(100);
    }
    
    // Update OLED display with all module data
    updateDisplay();
    
    Serial.println("---");
    delay(500);
}
