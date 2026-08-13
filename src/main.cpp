#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <esp_task_wdt.h>

#include "activity_classifier.h"
#include "alert_manager.h"
#include "config.h"
#include "config_manager.h"
#include "data_logger.h"
#include "drivers/cc1101.h"
#include "drivers/rx5808.h"
#include "drivers/sx1281.h"
#include "espnow_mesh.h"
#include "gps_module.h"
#include "power_manager.h"
#include "remote_id_detector.h"
#include "spi_manager.h"
#include "web_server.h"

namespace {

constexpr uint8_t kReceiverCount = 3;
constexpr uint8_t kSubGhzReceiver = 0;
constexpr uint8_t kTwoPointFourReceiver = 1;
constexpr uint8_t kFivePointEightReceiver = 2;

struct RFModuleData {
    const char* name;
    int rssi;
    bool active;
};

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0, U8X8_PIN_NONE, PIN_I2C_SCL, PIN_I2C_SDA);
CC1101Driver cc1101(PIN_CC1101_CS);
SX1281Driver sx1281;
RX5808Driver rx5808(PIN_RX5808_CH1, PIN_RX5808_CH2, PIN_RX5808_CH3,
                    PIN_RX5808_RSSI);
ActivityClassifier activityClassifier;
#if defined(MODULE_REMOTE_ID) && ENABLE_EXPERIMENTAL_REMOTE_ID
RemoteIDDetector remoteIDDetector;
#endif
SkySweepWebServer webServer;
DataLogger dataLogger;
GPSModule gpsModule;

volatile RFModuleData receivers[kReceiverCount] = {
    {"855-925", 0, false},
    {"2.4 GHz", 0, false},
    {"5.8 GHz", 0, false},
};

TaskHandle_t rfTask = nullptr;
TaskHandle_t displayTask = nullptr;
TaskHandle_t webTask = nullptr;
TaskHandle_t gpsTask = nullptr;

int readReceiverRssi(uint8_t receiver) {
    if (receiver == kFivePointEightReceiver) {
        return constrain(rx5808.scanNextChannel(), 0, 100);
    }

    if (!spiManager.acquire(pdMS_TO_TICKS(100))) {
        return 0;
    }

    int rssi = 0;
    if (receiver == kSubGhzReceiver) {
        rssi = map(cc1101.readRSSI(), -120, -30, 0, 100);
    } else if (receiver == kTwoPointFourReceiver) {
        rssi = sx1281.readRSSI();
    }
    spiManager.release();
    return constrain(rssi, 0, 100);
}

float receiverFrequencyMHz(uint8_t receiver) {
    if (receiver == kSubGhzReceiver) {
        return static_cast<float>(cc1101.getFrequency()) / 1000000.0f;
    }
    if (receiver == kTwoPointFourReceiver) {
        return sx1281.getCurrentFrequencyMHz();
    }
    return rx5808.getCurrentFrequency();
}

void taskRFScanning(void*) {
    Serial.println("[TASK] RF scanning started");
    uint8_t sweepCounter = 0;

    for (;;) {
        for (uint8_t receiver = 0; receiver < kReceiverCount; ++receiver) {
            receivers[receiver].rssi = readReceiverRssi(receiver);
            receivers[receiver].active = receivers[receiver].rssi > 40;

            // This is relative receiver activity only. It never identifies a
            // transmitter, protocol, range, intent, or aircraft.
            const ActivityLevel activity =
                activityClassifier.assessActivity(receiver, receivers[receiver].rssi);
            if (activity >= ACTIVITY_MEDIUM) {
                const AlertType alert = activity == ACTIVITY_CRITICAL
                                            ? ALERT_ACTIVITY_CRITICAL
                                            : activity == ACTIVITY_HIGH
                                                  ? ALERT_ACTIVITY_HIGH
                                                  : ALERT_ACTIVITY_MEDIUM;
                if (alertManager.getCurrentAlert() < alert ||
                    alertManager.getCurrentAlert() == ALERT_NONE) {
                    alertManager.alert(alert);
                }
            }

            if (activity >= ACTIVITY_HIGH) {
                // No protocol or transmitter identity is sent between nodes.
                espNowMesh.sendActivityAlert(receivers[receiver].rssi,
                                              receiver == kSubGhzReceiver ? 2 : receiver == kTwoPointFourReceiver ? 3 : 4,
                                              static_cast<uint8_t>(activity), 0.0f, 0.0f);
            }

            webServer.broadcastRFData(receivers[receiver].name, receivers[receiver].rssi,
                                      receivers[receiver].active);
            if (receivers[receiver].active) {
                dataLogger.logRFData(receivers[receiver].name, receivers[receiver].rssi,
                                     receiverFrequencyMHz(receiver), "energy");
            }
        }

        const ActivityData current = activityClassifier.getCurrentActivity();
        const char* source = current.isActive && current.moduleIndex < kReceiverCount
                                 ? receivers[current.moduleIndex].name
                                 : "All fitted receivers";
        webServer.broadcastActivityLevel(
            activityClassifier.getActivityLevelString(current.level), source);

        // The fitted E07 / CC1101 path remains within the Rev C 855–925 MHz
        // contract. It deliberately does not re-enable the legacy 433 MHz scan.
        if (++sweepCounter == 10) {
            sweepCounter = 0;
            if (spiManager.acquire(pdMS_TO_TICKS(100))) {
                const CC1101Driver::BandScanResult scan = cc1101.scanAllBands(5);
                spiManager.release();
                for (uint8_t band = 0; band < CC1101Driver::BAND_COUNT; ++band) {
                    if (scan.activity[band]) {
                        Serial.printf("[SWEEP] sample %u RSSI=%d\n", band,
                                      scan.rssi[band]);
                    }
                }
            }
        }

        powerManager.update();
        vTaskDelay(pdMS_TO_TICKS(configManager.get().rfScanIntervalMs));
    }
}

void taskDisplayUpdate(void*) {
    Serial.println("[TASK] Display update started");
    bool previousStealth = false;

    for (;;) {
        const bool stealth = configManager.get().stealthMode;
        if (stealth != previousStealth) {
            display.setPowerSave(stealth ? 1 : 0);
            previousStealth = stealth;
        }
        if (!stealth) {
            display.clearBuffer();
            display.setFont(u8g2_font_6x10_tr);
            display.drawStr(0, 10, "SkySweep32 Rev C");
            display.drawLine(0, 12, 128, 12);
            display.setFont(u8g2_font_5x7_tr);
            for (uint8_t receiver = 0; receiver < kReceiverCount; ++receiver) {
                const int y = 24 + receiver * 11;
                char row[28];
                snprintf(row, sizeof(row), "%s: %d", receivers[receiver].name,
                         receivers[receiver].rssi);
                display.drawStr(0, y, row);
                display.drawFrame(70, y - 7, 57, 8);
                const int width = map(receivers[receiver].rssi, 0, 100, 0, 55);
                if (width > 0) display.drawBox(71, y - 6, width, 6);
            }
            const ActivityData activity = activityClassifier.getCurrentActivity();
            if (activity.isActive) {
                char row[32];
                snprintf(row, sizeof(row), "ACTIVITY: %s",
                         activityClassifier.getActivityLevelString(activity.level));
                display.drawStr(0, 63, row);
            } else if (gpsModule.isFixValid()) {
                const GPSData gps = gpsModule.getData();
                char row[32];
                snprintf(row, sizeof(row), "GNSS: %d sat", gps.satellites);
                display.drawStr(0, 63, row);
            } else {
                display.drawStr(0, 63, "Scanning...");
            }
            display.sendBuffer();
        }
        vTaskDelay(pdMS_TO_TICKS(configManager.get().displayUpdateMs));
    }
}

void taskWebServer(void*) {
    Serial.println("[TASK] Web server started");
    for (;;) {
        webServer.update();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

#if defined(MODULE_REMOTE_ID) && ENABLE_EXPERIMENTAL_REMOTE_ID
void taskRemoteID(void*) {
    Serial.println("[TASK] Experimental Remote ID scanning started");
    for (;;) {
        remoteIDDetector.update();
        const uint8_t count = remoteIDDetector.getDetectedDroneCount();
        for (uint8_t index = 0; index < count; ++index) {
            const DroneRemoteIDData report = remoteIDDetector.getDroneData(index);
            if (report.isValid) {
                webServer.broadcastDroneDetection(report.uasID, report.latitude,
                                                  report.longitude, report.altitude);
                dataLogger.logDroneRemoteID(report.uasID, report.latitude,
                                             report.longitude, report.altitude,
                                             report.rssi);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif

void taskGPS(void*) {
    Serial.println("[TASK] GNSS polling started");
    for (;;) {
        gpsModule.update();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void beginReceiver(const char* name, bool initialized) {
    Serial.printf("[INIT] %s: %s\n", name, initialized ? "ready" : "failed");
}
void showInitialNetworkCredentials() {
    if (!configManager.networkCredentialsWereGenerated()) return;
    const RuntimeConfig& network = configManager.get();
    display.clearBuffer();
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(0, 10, "New AP credentials");
    display.drawStr(0, 25, network.wifiSSID);
    display.drawStr(0, 40, network.wifiPassword);
    display.drawStr(0, 55, "Serial has a copy");
    display.sendBuffer();
    delay(5000);
}


}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.printf("\nSkySweep32 Rev C passive monitor %s (%s)\n", SKYSWEEP_VERSION,
                  SKYSWEEP_BUILD_DATE);
    Serial.println("Relative RF energy/activity observation only; no transmitter identity.");

    const bool configReady = configManager.begin();
    esp_task_wdt_init(30, true);
    esp_task_wdt_add(nullptr);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    powerManager.begin();
    spiManager.begin();

    display.begin();
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 10, "SkySweep32");
    display.drawStr(0, 25, "Initializing...");
    display.sendBuffer();
    showInitialNetworkCredentials();
    if (spiManager.acquire(pdMS_TO_TICKS(1000))) {
        beginReceiver("CC1101 855-925 MHz", cc1101.begin());
        spiManager.release();
    }
    if (spiManager.acquire(pdMS_TO_TICKS(1000))) {
        beginReceiver("SX1281 2.4 GHz", sx1281.begin());
        spiManager.release();
    }
    beginReceiver("RX5808 5.8 GHz", rx5808.begin());

    alertManager.begin(true, true);
    if (configReady && webServer.begin()) {
        Serial.printf("[WEB] %s\n", webServer.getIPAddress().toString().c_str());
    } else if (!configReady) {
        Serial.println("[INIT] Web server disabled: configuration storage unavailable");
    } else {
        Serial.println("[INIT] Web server failed");
    }
    if (!espNowMesh.begin()) Serial.println("[INIT] ESP-NOW failed");
#if defined(MODULE_REMOTE_ID) && ENABLE_EXPERIMENTAL_REMOTE_ID
    if (!remoteIDDetector.begin()) Serial.println("[INIT] Experimental Remote ID failed");
#endif
    if (!dataLogger.begin(PIN_SD_CS)) Serial.println("[INIT] microSD unavailable");
    if (!gpsModule.begin()) Serial.println("[INIT] GNSS unavailable");

    xTaskCreatePinnedToCore(taskRFScanning, "RF_Scan", TASK_STACK_RF_SCAN, nullptr,
                            TASK_PRIORITY_RF_SCAN, &rfTask, 0);
    xTaskCreatePinnedToCore(taskDisplayUpdate, "Display", TASK_STACK_DISPLAY, nullptr,
                            TASK_PRIORITY_DISPLAY, &displayTask, 1);
    xTaskCreatePinnedToCore(taskWebServer, "Web", TASK_STACK_WEBSERVER, nullptr,
                            TASK_PRIORITY_WEBSERVER, &webTask, 1);
#if defined(MODULE_REMOTE_ID) && ENABLE_EXPERIMENTAL_REMOTE_ID
    xTaskCreatePinnedToCore(taskRemoteID, "RemoteID", TASK_STACK_REMOTE_ID, nullptr,
                            TASK_PRIORITY_REMOTE_ID, nullptr, 0);
#endif
    xTaskCreatePinnedToCore(taskGPS, "GNSS", TASK_STACK_GPS, nullptr,
                            TASK_PRIORITY_GPS, &gpsTask, 1);

    Serial.printf("[SYSTEM] Free heap: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
    esp_task_wdt_reset();
    alertManager.update();
    espNowMesh.update();
    vTaskDelay(pdMS_TO_TICKS(10));
}
