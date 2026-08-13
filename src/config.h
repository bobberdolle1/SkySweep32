#ifndef CONFIG_H
#define CONFIG_H

// Canonical SkySweep32 firmware configuration. This source tree targets the
// reviewed Rev C passive monitor only; legacy DevKit and Rev B wiring live in
// Git history and must not be compiled for a Rev C board.
#ifndef BOARD_SKYSWEEP32_REV_C
#error "Build SkySweep32 with PlatformIO environment esp32s3_rev_c_passive"
#endif

#include "generated/hardware_rev_c.h"

// Network defaults. Change runtime values through the local web UI after the
// first physical bring-up; the firmware must not imply that a radio path has
// been calibrated merely because it is enabled here.
#define WIFI_AP_SSID "SkySweep32"
#define WIFI_AP_PASSWORD "skysweep32"
#define WIFI_AP_CHANNEL 6
#define WIFI_MAX_CLIENTS 4
#define WEB_SERVER_PORT 80

// Relative activity thresholds for normalized 0–100 receiver energy/RSSI.
// They are not transmitter identity, range, intent, or threat thresholds.
#define RSSI_THRESHOLD_LOW 45
#define RSSI_THRESHOLD_MEDIUM 60
#define RSSI_THRESHOLD_HIGH 75
#define RSSI_THRESHOLD_CRITICAL 85

#define RF_SCAN_INTERVAL_MS 100
#define DISPLAY_UPDATE_INTERVAL_MS 500
#define WEB_BROADCAST_INTERVAL_MS 500
#define ACTIVITY_TIMEOUT_MS 5000
#define BLE_SCAN_INTERVAL_MS 5000
#define REMOTE_ID_CLEANUP_MS 30000

#define MAX_DETECTED_DRONES 20
#define MAX_WEBSOCKET_CLIENTS 4
#define MAX_LOG_SIZE_MB 10
#define MAX_LOG_FILES 50
#define RSSI_HISTORY_SIZE 32
#define LOG_DIR "/logs"

#define TASK_STACK_RF_SCAN 4096
#define TASK_STACK_DISPLAY 2048
#define TASK_STACK_WEBSERVER 8192
#define TASK_STACK_REMOTE_ID 4096
#define TASK_STACK_GPS 2048
#define TASK_STACK_DATALOG 3072

#define TASK_PRIORITY_RF_SCAN 3
#define TASK_PRIORITY_DISPLAY 1
#define TASK_PRIORITY_WEBSERVER 2
#define TASK_PRIORITY_REMOTE_ID 1
#define TASK_PRIORITY_GPS 1
#define TASK_PRIORITY_DATALOG 1

#define SKYSWEEP_VERSION "0.7.0-dev"
#define SKYSWEEP_BUILD_DATE __DATE__

#define RF_MODULE_COUNT 3

#endif  // CONFIG_H
