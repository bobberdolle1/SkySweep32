#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// SkySweep32 Configuration File
// Central configuration for all modules, pins, and feature flags
// ============================================================================

// ============================================================================
// TIER SELECTION — Choose your hardware configuration
// Uncomment ONE tier, or define via platformio.ini build_flags
// ============================================================================

// #define TIER_BASE       // ESP32 + OLED + NRF24L01+ (~$15-20)
// #define TIER_STANDARD   // Base + CC1101 + RX5808 (~$35-45)
// #define TIER_PRO        // Standard + GPS + SD + LoRa (~$60-80)
// #define TIER_JUGGERNAUT // Pro + 4x VCO Jammers (~$100+)

// Default to TIER_STANDARD if nothing defined
#if !defined(TIER_BASE) && !defined(TIER_STANDARD) && !defined(TIER_PRO) && !defined(TIER_JUGGERNAUT)
    #define TIER_STANDARD
#endif

// ============================================================================
// MODULE FLAGS — Auto-configured by tier, or override individually
// ============================================================================

// --- Tier: Base ---
#if defined(TIER_BASE) || defined(TIER_STANDARD) || defined(TIER_PRO)
    #ifndef MODULE_NRF24
        #define MODULE_NRF24              // 2.4 GHz monitoring (NRF24L01+)
    #endif
    #ifndef MODULE_OLED
        #define MODULE_OLED               // 128x64 OLED display
    #endif
    #ifndef MODULE_WEB_SERVER
        #define MODULE_WEB_SERVER         // WiFi AP + Web Dashboard
    #endif
    #ifndef MODULE_REMOTE_ID
        #define MODULE_REMOTE_ID          // BLE Remote ID (free, uses ESP32 BLE)
    #endif
#endif

// --- Tier: Standard (adds RF modules) ---
#if defined(TIER_STANDARD) || defined(TIER_PRO)
    #ifndef MODULE_CC1101
        #define MODULE_CC1101             // 900 MHz monitoring (CC1101)
    #endif
    #ifndef MODULE_RX5808
        #define MODULE_RX5808             // 5.8 GHz monitoring (RX5808)
    #endif
#endif

// --- Tier: Pro & Juggernaut (adds GPS, SD, LoRa) ---
#if defined(TIER_PRO) || defined(TIER_JUGGERNAUT)
    #ifndef MODULE_GPS
        #define MODULE_GPS                // GPS geolocation (NEO-6M/7M)
    #endif
    #ifndef MODULE_SD_CARD
        #define MODULE_SD_CARD            // SD card forensic logging
    #endif
    #ifndef MODULE_LORA
        #define MODULE_LORA               // LoRa mesh networking (SX1276)
    #endif
#endif

// --- Tier: Juggernaut (adds EW) ---
#if defined(TIER_JUGGERNAUT)
    #ifndef ENABLE_COUNTERMEASURES
        #define ENABLE_COUNTERMEASURES    // Enable DAC VCO jamming
    #endif
#endif

// --- Optional modules (enable manually in any tier) ---
// #define MODULE_ACOUSTIC            // MEMS microphone acoustic detection (~$5)
// #define MODULE_ML                  // ML drone classification (rule-based + optional TFLite)
// #define ENABLE_COUNTERMEASURES     // Active countermeasures (LEGAL AUTH REQUIRED!)

// Auto-enable ML if any RF module is active
#if defined(MODULE_CC1101) || defined(MODULE_NRF24) || defined(MODULE_RX5808)
    #ifndef MODULE_ML
        #define MODULE_ML
    #endif
#endif

// ============================================================================
// PIN DEFINITIONS — Conflict-free pin map for ESP32 DevKit v1
// ============================================================================

// --- SPI Bus (shared by all SPI devices) ---
#define PIN_SPI_MOSI        23
#define PIN_SPI_MISO        19
#define PIN_SPI_SCK         18

// --- CC1101 (900 MHz) ---
#define PIN_CC1101_CS       5

// --- NRF24L01+ (2.4 GHz) ---
#define PIN_NRF24_CS        15
#define PIN_NRF24_CE        2

// --- RX5808 (5.8 GHz) ---
#define PIN_RX5808_CS       13
#define PIN_RX5808_RSSI     34    // ADC1_CH6 (analog input)

// --- OLED Display (I2C) ---
#define PIN_I2C_SDA         21
#define PIN_I2C_SCL         22

// --- GPS Module (UART2) ---
#define PIN_GPS_RX          16    // ESP32 RX ← GPS TX
#define PIN_GPS_TX          17    // ESP32 TX → GPS RX
#define GPS_BAUD_RATE       9600
#define GPS_UPDATE_INTERVAL 1000

// --- LoRa SX1276 ---
#define PIN_LORA_CS         14    // Changed from 26
#define PIN_LORA_DIO0       33
#define PIN_LORA_DIO1       32
#define PIN_LORA_RESET      12    // Changed from 25

// --- VCO DACs (Juggernaut) ---
#define PIN_VCO_DAC_1       25
#define PIN_VCO_DAC_2       26

// --- SD Card ---
#define PIN_SD_CS           27

// --- I2S Microphone (Acoustic Detection) ---
#define PIN_I2S_BCLK        14
#define PIN_I2S_WS          12
#define PIN_I2S_DIN         35    // ADC1_CH7 (input only pin)

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================

#define WIFI_AP_SSID        "SkySweep32"
#define WIFI_AP_PASSWORD    "skysweep32"
#define WIFI_AP_CHANNEL     6
#define WIFI_MAX_CLIENTS    4
#define WEB_SERVER_PORT     80

// ============================================================================
// RSSI THRESHOLDS (Threat Assessment)
// ============================================================================

#define RSSI_THRESHOLD_LOW          45
#define RSSI_THRESHOLD_MEDIUM       60
#define RSSI_THRESHOLD_HIGH         75
#define RSSI_THRESHOLD_CRITICAL     85

// ============================================================================
// TIMING CONSTANTS
// ============================================================================

#define RF_SCAN_INTERVAL_MS         100   // RF polling interval
#define DISPLAY_UPDATE_INTERVAL_MS  500   // OLED refresh rate
#define WEB_BROADCAST_INTERVAL_MS   500   // WebSocket update rate
#define THREAT_TIMEOUT_MS           5000  // Threat clear timeout
#define BLE_SCAN_INTERVAL_MS        5000  // Remote ID BLE scan
#define REMOTE_ID_CLEANUP_MS        30000 // Remove stale detections

// ============================================================================
// LIMITS
// ============================================================================

#define MAX_DETECTED_DRONES         20    // Max drones in memory
#define MAX_WEBSOCKET_CLIENTS       4     // Max concurrent WS connections
#define MAX_LOG_SIZE_MB             10    // Max single log file size
#define MAX_LOG_FILES               50    // Max log files before rotation
#define RSSI_HISTORY_SIZE           32    // RSSI history buffer

// ============================================================================
// LORA CONFIGURATION
// ============================================================================

#define LORA_FREQUENCY          915.0   // MHz (US ISM band)
#define LORA_BANDWIDTH          125.0   // kHz
#define LORA_SPREADING_FACTOR   7
#define LORA_CODING_RATE        5
#define LORA_SYNC_WORD          0x12
#define LORA_TX_POWER           20      // dBm
#define LORA_TRANSMIT_INTERVAL  30000   // ms between broadcasts

// ============================================================================
// ML CLASSIFIER
// ============================================================================

#define ML_INPUT_SIZE           128
#define ML_OUTPUT_SIZE          5
#define ML_INFERENCE_THRESHOLD  0.6f

// ============================================================================
// DATA LOGGER
// ============================================================================

#define LOG_DIR                 "/logs"

// ============================================================================
// FREERTOS TASK CONFIGURATION
// ============================================================================

#define TASK_STACK_RF_SCAN      4096
#define TASK_STACK_PROTOCOL     3072
#define TASK_STACK_THREAT       3072
#define TASK_STACK_DISPLAY      2048
#define TASK_STACK_WEBSERVER    8192
#define TASK_STACK_REMOTE_ID    4096
#define TASK_STACK_GPS          2048
#define TASK_STACK_DATALOG      3072
#define TASK_STACK_MESH         4096
#define TASK_STACK_ACOUSTIC     4096

// Task priorities (higher = more important)
#define TASK_PRIORITY_RF_SCAN   3
#define TASK_PRIORITY_PROTOCOL  2
#define TASK_PRIORITY_THREAT    3
#define TASK_PRIORITY_DISPLAY   1
#define TASK_PRIORITY_WEBSERVER 2
#define TASK_PRIORITY_REMOTE_ID 1
#define TASK_PRIORITY_GPS       1
#define TASK_PRIORITY_DATALOG   1
#define TASK_PRIORITY_MESH      1
#define TASK_PRIORITY_ACOUSTIC  2

// ============================================================================
// VERSION
// ============================================================================

#define SKYSWEEP_VERSION        "0.4.0"
#define SKYSWEEP_BUILD_DATE     __DATE__

// ============================================================================
// ACTIVE MODULE COUNT (computed)
// ============================================================================

#define _RF_COUNT_BASE  0
#ifdef MODULE_CC1101
    #define _RF_COUNT_CC  (_RF_COUNT_BASE + 1)
#else
    #define _RF_COUNT_CC  _RF_COUNT_BASE
#endif
#ifdef MODULE_NRF24
    #define _RF_COUNT_NRF (_RF_COUNT_CC + 1)
#else
    #define _RF_COUNT_NRF _RF_COUNT_CC
#endif
#ifdef MODULE_RX5808
    #define RF_MODULE_COUNT (_RF_COUNT_NRF + 1)
#else
    #define RF_MODULE_COUNT _RF_COUNT_NRF
#endif

#endif // CONFIG_H
