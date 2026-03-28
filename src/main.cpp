#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "config.h"
#include "config_manager.h"
#include "power_manager.h"
#include "spi_manager.h"
#include "countermeasures.h"
#include "alert_manager.h"
#include "espnow_mesh.h"
#include "signal_database.h"
#include <esp_task_wdt.h>

// --- Conditionally include modules ---
#ifdef MODULE_CC1101
#include "drivers/cc1101.h"
#endif
#ifdef MODULE_NRF24
#include "drivers/nrf24l01.h"
#endif
#ifdef MODULE_RX5808
#include "drivers/rx5808.h"
#endif
#include "protocols/mavlink_parser.h"
#include "protocols/crsf_parser.h"
#ifdef MODULE_ACOUSTIC
#include "acoustic_detector.h"
#endif
#ifdef MODULE_REMOTE_ID
#include "remote_id_detector.h"
#endif
#ifdef MODULE_WEB_SERVER
#include "web_server.h"
#endif
#ifdef MODULE_SD_CARD
#include "data_logger.h"
#endif
#ifdef MODULE_GPS
#include "gps_module.h"
#endif
#ifdef MODULE_LORA
#include "meshtastic_client.h"
#endif
#ifdef MODULE_ML
#include "ml_classifier.h"
#endif

// ============================================================================
// DISPLAY
// ============================================================================
#ifdef MODULE_OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, PIN_I2C_SCL, PIN_I2C_SDA);
#endif

// ============================================================================
// RF MODULE DRIVER INSTANCES
// ============================================================================
#ifdef MODULE_CC1101
CC1101Driver cc1101(PIN_CC1101_CS);
#endif
#ifdef MODULE_NRF24
NRF24L01Driver nrf24(PIN_NRF24_CS, PIN_NRF24_CE);
#endif
#ifdef MODULE_RX5808
RX5808Driver rx5808(PIN_RX5808_CS, PIN_RX5808_RSSI);
#endif

// Protocol Parsers
MAVLinkParser mavlinkParser;
CRSFParser crsfParser;

// ============================================================================
// FEATURE MODULE INSTANCES
// ============================================================================
CountermeasureSystem counterMeasures;

#ifdef MODULE_REMOTE_ID
RemoteIDDetector remoteIDDetector;
#endif
#ifdef MODULE_WEB_SERVER
SkySweepWebServer webServer;
#endif
#ifdef MODULE_SD_CARD
DataLogger dataLogger;
#endif
#ifdef MODULE_GPS
GPSModule gpsModule;
#endif
#ifdef MODULE_LORA
MeshtasticClient meshtasticClient;
#endif
#ifdef MODULE_ML
MLClassifier mlClassifier;
#endif
#ifdef MODULE_ACOUSTIC
AcousticDetector acousticDetector;
#endif

// ============================================================================
// SHARED DATA (protected by mutexes)
// ============================================================================

struct RFModuleData {
    const char* moduleName;
    uint8_t chipSelectPin;
    int rssiValue;
    bool isActive;
    bool protocolMAVLink;
    bool protocolCRSF;
};

// RF data accessible from multiple tasks
volatile RFModuleData rfModules[3] = {
    {"CC1101",    PIN_CC1101_CS,   0, false, false, false},
    {"NRF24L01+", PIN_NRF24_CS,   0, false, false, false},
    {"RX5808",    PIN_RX5808_CS,  0, false, false, false}
};

// RSSI history for ML classifier
int rssiHistory[RSSI_HISTORY_SIZE] = {0};
uint8_t rssiHistoryIndex = 0;

// Task handles
TaskHandle_t taskHandleRF = NULL;
TaskHandle_t taskHandleDisplay = NULL;
TaskHandle_t taskHandleWeb = NULL;
TaskHandle_t taskHandleRemoteID = NULL;
TaskHandle_t taskHandleGPS = NULL;

// ============================================================================
// RF SCANNING TASK (Core 0)
// ============================================================================

int readModuleRSSI(uint8_t moduleIndex) {
    int rssiValue = 0;
    
    if (!spiManager.acquire(pdMS_TO_TICKS(100))) return 0;
    
    switch(moduleIndex) {
        case 0:
            #ifdef MODULE_CC1101
            rssiValue = cc1101.readRSSI();
            rssiValue = map(rssiValue, -120, -30, 0, 100);
            #endif
            break;
        case 1:
            #ifdef MODULE_NRF24
            rssiValue = nrf24.readRSSI();
            rssiValue = map(rssiValue, -90, -40, 0, 100);
            #endif
            break;
        case 2:
            #ifdef MODULE_RX5808
            rssiValue = rx5808.readRSSI();
            #endif
            break;
    }
    
    spiManager.release();
    return constrain(rssiValue, 0, 100);
}

void taskRFScanning(void* parameter) {
    Serial.println("[TASK] RF Scanning started (Core 0)");
    
    for (;;) {
        // Scan each active RF module
        for (uint8_t i = 0; i < 3; i++) {
            // Check if module is enabled
            #ifndef MODULE_CC1101
            if (i == 0) continue;
            #endif
            #ifndef MODULE_NRF24
            if (i == 1) continue;
            #endif
            #ifndef MODULE_RX5808
            if (i == 2) continue;
            #endif
            
            // Read RSSI
            rfModules[i].rssiValue = readModuleRSSI(i);
            rfModules[i].isActive = (rfModules[i].rssiValue > 40);
            
            // Store history for ML
            rssiHistory[rssiHistoryIndex] = rfModules[i].rssiValue;
            rssiHistoryIndex = (rssiHistoryIndex + 1) % RSSI_HISTORY_SIZE;
            
            // Try protocol parsing on received data
            if (rfModules[i].isActive) {
                uint8_t rxBuf[64];
                uint8_t rxLen = 0;
                
                if (spiManager.acquire(pdMS_TO_TICKS(50))) {
                    switch(i) {
                        case 0:
                            #ifdef MODULE_CC1101
                            rxLen = cc1101.receiveData(rxBuf, sizeof(rxBuf));
                            #endif
                            break;
                        case 1:
                            #ifdef MODULE_NRF24
                            rxLen = nrf24.receiveData(rxBuf, sizeof(rxBuf));
                            #endif
                            break;
                    }
                    spiManager.release();
                }
                
                // Parse protocols
                if (rxLen > 0) {
                    if (mavlinkParser.parseBuffer(rxBuf, rxLen)) {
                        rfModules[i].protocolMAVLink = true;
                        MAVLinkPacket pkt = mavlinkParser.getPacket();
                        Serial.printf("[PROTO] MAVLink detected on %s: %s (SysID:%d)\n",
                                     rfModules[i].moduleName,
                                     mavlinkParser.getMessageName(pkt.msgid),
                                     pkt.sysid);
                    }
                    if (crsfParser.parseBuffer(rxBuf, rxLen)) {
                        rfModules[i].protocolCRSF = true;
                        CRSFPacket pkt = crsfParser.getPacket();
                        Serial.printf("[PROTO] CRSF detected on %s: %s\n",
                                     rfModules[i].moduleName,
                                     crsfParser.getFrameTypeName(pkt.type));
                    }
                }
            }
            
            // Threat assessment
            ThreatLevel threat = counterMeasures.assessThreat(i, rfModules[i].rssiValue);
            
            // Trigger alerts
            if (threat >= THREAT_MEDIUM) {
                AlertType requiredAlert = (threat == THREAT_CRITICAL) ? ALERT_THREAT_CRITICAL :
                                          (threat == THREAT_HIGH) ? ALERT_THREAT_HIGH : ALERT_DRONE_DETECTED;
                if (alertManager.getCurrentAlert() < requiredAlert || alertManager.getCurrentAlert() == ALERT_NONE) {
                    alertManager.alert(requiredAlert);
                }
            }
            
            // Signal Signature Matching
            bool bands[5] = {false};
            if (i == 0) bands[RF_BAND_915] = true;  // Simplified mapping
            if (i == 1) bands[RF_BAND_2400] = true;
            if (i == 2) bands[RF_BAND_5800] = true;
            
            SignatureMatch sigMatch = signalDB.matchSignal(bands, rfModules[i].rssiValue, 5.0f, 
                                                           rfModules[i].protocolMAVLink, 
                                                           rfModules[i].protocolCRSF, false);
            
            if (sigMatch.confidence > 0.6f && threat >= THREAT_LOW) {
                Serial.printf("[SIGDB] Match: %s (%.0f%%)\n", sigMatch.name, sigMatch.confidence * 100.0f);
            }
            
            // ESP-NOW Mesh: Broadcast threats to nearby nodes
            if (threat >= THREAT_HIGH) {
                espNowMesh.sendThreatAlert(rfModules[i].rssiValue,
                                           i == 0 ? 2 : (i == 1 ? 3 : 4), // 915=2, 2.4=3, 5.8=4
                                           rfModules[i].protocolMAVLink ? 1 : rfModules[i].protocolCRSF ? 2 : 0,
                                           0.0, 0.0);
            }
            
            // ML classification on high threat (optional, kept from previous)
            #ifdef MODULE_ML
            if (threat >= THREAT_HIGH) {
                ClassificationResult mlResult = mlClassifier.classifyFromRSSI(
                    rssiHistory, RSSI_HISTORY_SIZE,
                    rfModules[i].protocolMAVLink,
                    rfModules[i].protocolCRSF,
                    rfModules[0].isActive,  // 900 MHz
                    rfModules[1].isActive,  // 2.4 GHz
                    rfModules[2].isActive   // 5.8 GHz
                );
                if (mlResult.isValid) {
                    Serial.printf("[ML] %s (%.0f%% confidence)\n", 
                                 mlClassifier.getClassName(mlResult.droneClass), 
                                 mlResult.confidence * 100.0f);
                }
            }
            #endif
            
            // Broadcast to web clients
            #ifdef MODULE_WEB_SERVER
            webServer.broadcastRFData(rfModules[i].moduleName, rfModules[i].rssiValue, rfModules[i].isActive);
            if (threat >= THREAT_LOW) {
                webServer.broadcastThreatLevel(
                    counterMeasures.getThreatLevelString(threat),
                    counterMeasures.getProtocolString(counterMeasures.getCurrentThreat().detectedProtocol)
                );
            }
            #endif
            
            // Log detection to SD
            #ifdef MODULE_SD_CARD
            if (rfModules[i].isActive) {
                float freq = (i == 0) ? 915.0f : (i == 1) ? 2400.0f : 5800.0f;
                const char* proto = rfModules[i].protocolMAVLink ? "MAVLink" :
                                    rfModules[i].protocolCRSF ? "CRSF" : "Unknown";
                dataLogger.logRFData(rfModules[i].moduleName, rfModules[i].rssiValue, freq, proto);
            }
            #endif
            
            // LoRa alert broadcast on critical threat
            #ifdef MODULE_LORA
            if (threat >= THREAT_CRITICAL) {
                DetectionAlert alert = {};
                #ifdef MODULE_GPS
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
            
            // Auto-respond with countermeasures if armed
            #ifdef ENABLE_COUNTERMEASURES
            if (counterMeasures.isArmed() && threat >= THREAT_HIGH) {
                if (spiManager.acquire(pdMS_TO_TICKS(200))) {
                    counterMeasures.autoRespond(i, rfModules[i].rssiValue, rfModules[i].chipSelectPin);
                    spiManager.release();
                }
            }
            #endif
        }
        
        // --- Periodic multi-band sweep (every 10th cycle) ---
        static uint8_t sweepCounter = 0;
        if (++sweepCounter >= 10) {
            sweepCounter = 0;
        }

        #ifdef MODULE_CC1101
        if (sweepCounter == 0) {
            if (spiManager.acquire(pdMS_TO_TICKS(100))) {
                CC1101Driver::BandScanResult bandResult = cc1101.scanAllBands(5);
                spiManager.release();
                
                for (uint8_t b = 0; b < CC1101Driver::BAND_COUNT; b++) {
                    if (bandResult.activity[b]) {
                        Serial.printf("[SWEEP] Activity on %s: RSSI=%d\n",
                                     b == 0 ? "433MHz" : b == 1 ? "868MHz" : "915MHz",
                                     bandResult.rssi[b]);
                    }
                }
            }
        }
        #endif
        
        #ifdef MODULE_NRF24
        if (sweepCounter == 5) { // Staggered from CC1101 sweep
            if (spiManager.acquire(pdMS_TO_TICKS(100))) {
                NRF24L01Driver::ChannelScanResult nrfScan = nrf24.scanSpectrum(20, 200);
                spiManager.release();
                
                if (nrfScan.activeChannels > 10) {
                    Serial.printf("[NRF24-SWEEP] High 2.4GHz activity: %d channels active. Peak ch%d\n",
                                  nrfScan.activeChannels, nrfScan.peakChannel);
                }
            }
        }
        #endif
        
        // Power manager update (battery check)
        powerManager.update();
        
        vTaskDelay(pdMS_TO_TICKS(configManager.get().rfScanIntervalMs));
    }
}

// ============================================================================
// DISPLAY TASK (Core 1)
// ============================================================================

void taskDisplayUpdate(void* parameter) {
    Serial.println("[TASK] Display update started (Core 1)");
    
    for (;;) {
        #ifdef MODULE_OLED
        display.clearBuffer();
        
        // Header
        display.setFont(u8g2_font_6x10_tr);
        display.drawStr(0, 10, "SkySweep32");
        
        // Show tier
        #ifdef TIER_BASE
        display.drawStr(68, 10, "[BASE]");
        #elif defined(TIER_STANDARD)
        display.drawStr(68, 10, "[STD]");
        #elif defined(TIER_PRO)
        display.drawStr(68, 10, "[PRO]");
        #endif
        
        display.drawLine(0, 12, 128, 12);
        
        // Module Data
        display.setFont(u8g2_font_5x7_tr);
        int yPos = 24;
        
        for (int i = 0; i < 3; i++) {
            // Skip disabled modules
            #ifndef MODULE_CC1101
            if (i == 0) continue;
            #endif
            #ifndef MODULE_NRF24
            if (i == 1) continue;
            #endif
            #ifndef MODULE_RX5808
            if (i == 2) continue;
            #endif
            
            char buf[32];
            snprintf(buf, sizeof(buf), "%s:%d", rfModules[i].moduleName, rfModules[i].rssiValue);
            display.drawStr(0, yPos, buf);
            
            // Signal bar
            int barW = map(rfModules[i].rssiValue, 0, 100, 0, 55);
            display.drawFrame(70, yPos - 7, 57, 8);
            if (barW > 0) display.drawBox(71, yPos - 6, barW, 6);
            
            yPos += 11;
        }
        
        // Threat indicator
        ThreatData threat = counterMeasures.getCurrentThreat();
        display.setFont(u8g2_font_5x7_tr);
        if (threat.isActive) {
            char threatBuf[32];
            snprintf(threatBuf, sizeof(threatBuf), "THREAT: %s", 
                     counterMeasures.getThreatLevelString(threat.level));
            display.drawStr(0, 63, threatBuf);
        } else {
            #ifdef MODULE_GPS
            if (gpsModule.isFixValid()) {
                GPSData g = gpsModule.getData();
                char gpsBuf[32];
                snprintf(gpsBuf, sizeof(gpsBuf), "GPS:%dsat %.4f", g.satellites, g.latitude);
                display.drawStr(0, 63, gpsBuf);
            } else {
                display.drawStr(0, 63, "Scanning...");
            }
            #else
            display.drawStr(0, 63, "Scanning...");
            #endif
        }
        
        display.sendBuffer();
        #endif // MODULE_OLED
        
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_INTERVAL_MS));
    }
}

// ============================================================================
// WEB SERVER TASK (Core 1)
// ============================================================================

#ifdef MODULE_WEB_SERVER
void taskWebServer(void* parameter) {
    Serial.println("[TASK] Web server started (Core 1)");
    for (;;) {
        webServer.update();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif

// ============================================================================
// REMOTE ID TASK (Core 0)
// ============================================================================

#ifdef MODULE_REMOTE_ID
void taskRemoteID(void* parameter) {
    Serial.println("[TASK] Remote ID scanning started (Core 0)");
    for (;;) {
        remoteIDDetector.update();
        
        uint8_t droneCount = remoteIDDetector.getDetectedDroneCount();
        for (uint8_t i = 0; i < droneCount; i++) {
            DroneRemoteIDData drone = remoteIDDetector.getDroneData(i);
            if (drone.isValid) {
                #ifdef MODULE_WEB_SERVER
                webServer.broadcastDroneDetection(drone.uasID, drone.latitude, drone.longitude, drone.altitude);
                #endif
                #ifdef MODULE_SD_CARD
                dataLogger.logDroneRemoteID(drone.uasID, drone.latitude, drone.longitude, drone.altitude, drone.rssi);
                #endif
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif

// ============================================================================
// GPS TASK (Core 1)
// ============================================================================

#ifdef MODULE_GPS
void taskGPS(void* parameter) {
    Serial.println("[TASK] GPS polling started (Core 1)");
    for (;;) {
        gpsModule.update();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif

// ============================================================================
// LORA MESH TASK (Core 0)
// ============================================================================

#ifdef MODULE_LORA
void taskLoRaMesh(void* parameter) {
    Serial.println("[TASK] LoRa mesh started (Core 0)");
    for (;;) {
        if (spiManager.acquire(pdMS_TO_TICKS(100))) {
            meshtasticClient.update();
            spiManager.release();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
#endif

// ============================================================================
// ACOUSTIC DETECTION TASK (Core 0)
// ============================================================================

#ifdef MODULE_ACOUSTIC
void taskAcoustic(void* parameter) {
    Serial.println("[TASK] Acoustic detection started (Core 0)");
    for (;;) {
        acousticDetector.update();
        if (acousticDetector.isDroneDetected()) {
            Serial.printf("[ACOUSTIC] Drone detected! Ratio: %.6f\n", acousticDetector.getCurrentRatio());
        }
        vTaskDelay(pdMS_TO_TICKS(HOP_MS));
    }
}
#endif

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║   SkySweep32 Passive Drone Detector  ║");
    Serial.printf( "║   Version %-10s  %s  ║\n", SKYSWEEP_VERSION, SKYSWEEP_BUILD_DATE);
    Serial.println("╠══════════════════════════════════════╣");
    
    #ifdef TIER_BASE
    Serial.println("║   Tier: 🟢 BASE (Starter)            ║");
    #elif defined(TIER_STANDARD)
    Serial.println("║   Tier: 🟡 STANDARD (Hunter)         ║");
    #elif defined(TIER_PRO)
    Serial.println("║   Tier: 🔴 PRO (Sentinel)            ║");
    #endif
    
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // --- Initialize ConfigManager (SPIFFS) ---
    Serial.println("[INIT] Configuration manager...");
    configManager.begin();
    configManager.printConfig();
    
    // --- Initialize Watchdog Timer ---
    esp_task_wdt_init(30, true);  // 30s timeout, panic on trigger
    esp_task_wdt_add(NULL);       // Add current task (setup/loop)
    Serial.println("[INIT] Watchdog timer: 30s");
    
    // --- Initialize Power Manager ---
    powerManager.begin();
    
    // --- Initialize SPI Manager ---
    spiManager.begin();
    
    // --- Initialize I2C + OLED ---
    #ifdef MODULE_OLED
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    display.begin();
    display.setFont(u8g2_font_6x10_tr);
    display.clearBuffer();
    display.drawStr(0, 10, "SkySweep32");
    display.drawStr(0, 25, "Initializing...");
    display.sendBuffer();
    Serial.println("[INIT] OLED display ready");
    #endif
    
    // --- Initialize RF Modules ---
    #ifdef MODULE_CC1101
    Serial.println("[INIT] CC1101 (900 MHz)...");
    if (spiManager.acquire(pdMS_TO_TICKS(1000))) {
        if (!cc1101.begin()) Serial.println("[ERROR] CC1101 init failed");
        spiManager.release();
    }
    #endif
    
    #ifdef MODULE_NRF24
    Serial.println("[INIT] NRF24L01+ (2.4 GHz)...");
    pinMode(PIN_NRF24_CE, OUTPUT);
    if (spiManager.acquire(pdMS_TO_TICKS(1000))) {
        if (!nrf24.begin()) Serial.println("[ERROR] NRF24 init failed");
        spiManager.release();
    }
    #endif
    
    #ifdef MODULE_RX5808
    Serial.println("[INIT] RX5808 (5.8 GHz)...");
    if (spiManager.acquire(pdMS_TO_TICKS(1000))) {
        if (!rx5808.begin()) Serial.println("[ERROR] RX5808 init failed");
        spiManager.release();
    }
    #endif
    
    // --- Initialize Countermeasures ---
    counterMeasures.initialize();
    
    // --- Initialize Alert Manager ---
    alertManager.begin(true, true);
    
    // --- Initialize Signal Database ---
    signalDB.begin();
    
    // --- Initialize Web Server (before Remote ID!) ---
    #ifdef MODULE_WEB_SERVER
    Serial.println("[INIT] Web Server...");
    if (!webServer.begin(true)) {
        Serial.println("[ERROR] Web server init failed");
    } else {
        Serial.printf("[WEB] Dashboard: http://%s\n", webServer.getIPAddress().toString().c_str());
    }
    #endif
    
    // --- Initialize ESP-NOW Mesh ---
    Serial.println("[INIT] ESP-NOW Mesh...");
    if (!espNowMesh.begin()) {
        Serial.println("[ERROR] ESP-NOW init failed");
    }
    
    // --- Initialize Remote ID (after WiFi is configured) ---
    #ifdef MODULE_REMOTE_ID
    Serial.println("[INIT] Remote ID (BLE)...");
    if (!remoteIDDetector.begin()) {
        Serial.println("[ERROR] Remote ID init failed");
    }
    #endif
    
    // --- Initialize SD Card Logger ---
    #ifdef MODULE_SD_CARD
    Serial.println("[INIT] SD Card Logger...");
    if (spiManager.acquire(pdMS_TO_TICKS(1000))) {
        if (!dataLogger.begin(PIN_SD_CS)) {
            Serial.println("[ERROR] SD card init failed");
        }
        spiManager.release();
    }
    #endif
    
    // --- Initialize GPS ---
    #ifdef MODULE_GPS
    Serial.println("[INIT] GPS Module...");
    if (!gpsModule.begin()) {
        Serial.println("[ERROR] GPS init failed");
    }
    #endif
    
    // --- Initialize LoRa Mesh ---
    #ifdef MODULE_LORA
    Serial.println("[INIT] LoRa Mesh...");
    if (spiManager.acquire(pdMS_TO_TICKS(1000))) {
        if (!meshtasticClient.begin(LORA_FREQUENCY)) {
            Serial.println("[ERROR] LoRa init failed");
        }
        spiManager.release();
    }
    #endif
    
    // --- Initialize ML Classifier ---
    #ifdef MODULE_ML
    Serial.println("[INIT] ML Classifier...");
    mlClassifier.begin();
    #endif
    
    // --- Initialize Acoustic Detector ---
    #ifdef MODULE_ACOUSTIC
    Serial.println("[INIT] Acoustic Detector...");
    if (!acousticDetector.begin()) {
        Serial.println("[ERROR] Acoustic init failed");
    }
    #endif
    
    // ===================================================================
    // CREATE FREERTOS TASKS
    // ===================================================================
    
    Serial.println("\n[SYSTEM] Creating FreeRTOS tasks...");
    
    // RF Scanning — Core 0, highest priority
    xTaskCreatePinnedToCore(taskRFScanning, "RF_Scan", TASK_STACK_RF_SCAN, 
                            NULL, TASK_PRIORITY_RF_SCAN, &taskHandleRF, 0);
    
    // Display — Core 1
    #ifdef MODULE_OLED
    xTaskCreatePinnedToCore(taskDisplayUpdate, "Display", TASK_STACK_DISPLAY, 
                            NULL, TASK_PRIORITY_DISPLAY, &taskHandleDisplay, 1);
    #endif
    
    // Web Server — Core 1
    #ifdef MODULE_WEB_SERVER
    xTaskCreatePinnedToCore(taskWebServer, "WebSrv", TASK_STACK_WEBSERVER, 
                            NULL, TASK_PRIORITY_WEBSERVER, &taskHandleWeb, 1);
    #endif
    
    // Remote ID — Core 0
    #ifdef MODULE_REMOTE_ID
    xTaskCreatePinnedToCore(taskRemoteID, "RemoteID", TASK_STACK_REMOTE_ID, 
                            NULL, TASK_PRIORITY_REMOTE_ID, NULL, 0);
    #endif
    
    // GPS — Core 1
    #ifdef MODULE_GPS
    xTaskCreatePinnedToCore(taskGPS, "GPS", TASK_STACK_GPS, 
                            NULL, TASK_PRIORITY_GPS, &taskHandleGPS, 1);
    #endif
    
    // LoRa Mesh — Core 0
    #ifdef MODULE_LORA
    xTaskCreatePinnedToCore(taskLoRaMesh, "LoRa", TASK_STACK_MESH, 
                            NULL, TASK_PRIORITY_MESH, NULL, 0);
    #endif
    
    // Acoustic — Core 0
    #ifdef MODULE_ACOUSTIC
    xTaskCreatePinnedToCore(taskAcoustic, "Acoustic", TASK_STACK_ACOUSTIC, 
                            NULL, TASK_PRIORITY_ACOUSTIC, NULL, 0);
    #endif
    
    Serial.println("[SYSTEM] All tasks started");
    Serial.printf("[SYSTEM] Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.println("═══════════════════════════════════════\n");
}

// ============================================================================
// LOOP (minimal — tasks handle everything)
// ============================================================================

void loop() {
    // Keep watchdog fed
    esp_task_wdt_reset();
    
    // Fast non-blocking updates
    alertManager.update();
    espNowMesh.update();
    
    // Yield to FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(10));
}
