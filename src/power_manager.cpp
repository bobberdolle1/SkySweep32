#include "power_manager.h"
#include <esp_wifi.h>
#include <esp_bt.h>

PowerManager powerManager;

PowerManager::PowerManager()
    : currentMode(POWER_FULL),
      batteryVoltage(0.0f),
      batteryPercent(100),
      batteryMonitoring(false),
      lastBatteryRead(0),
      sleepDurationUs(60000000),     // 60 seconds default
      wakeScansBeforeSleep(3) {}

void PowerManager::begin() {
    // Check if woke from deep sleep
    if (isWakeFromSleep()) {
        Serial.println("[PWR] Woke from deep sleep");
    }
    
    // Configure battery ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    batteryMonitoring = true;
    
    // Initial battery read
    getBatteryVoltage();
    
    Serial.printf("[PWR] Battery: %.2fV (%d%%)\n", batteryVoltage, batteryPercent);
    Serial.printf("[PWR] Mode: %s\n", getModeName());
}

void PowerManager::update() {
    // Read battery every 10 seconds
    if (millis() - lastBatteryRead > 10000) {
        getBatteryVoltage();
        lastBatteryRead = millis();
        
        // Auto-switch to low power if battery critical
        if (isBatteryCritical() && currentMode != POWER_SLEEP) {
            Serial.println("[PWR] ⚠️ Battery critical! Switching to LOW power mode");
            setMode(POWER_LOW);
        }
    }
}

void PowerManager::setMode(PowerMode mode) {
    if (mode == currentMode) return;
    
    PowerMode prevMode = currentMode;
    currentMode = mode;
    
    switch (mode) {
        case POWER_FULL:
            setCPUFrequency(240);
            setWiFiPowerSave(false);
            Serial.println("[PWR] Mode: FULL (240 MHz, WiFi full power)");
            break;
            
        case POWER_BALANCED:
            setCPUFrequency(160);
            setWiFiPowerSave(true);
            Serial.println("[PWR] Mode: BALANCED (160 MHz, WiFi PS)");
            break;
            
        case POWER_LOW:
            setCPUFrequency(80);
            setWiFiPowerSave(true);
            Serial.println("[PWR] Mode: LOW (80 MHz, WiFi PS, reduced scan)");
            break;
            
        case POWER_SLEEP:
            Serial.println("[PWR] Mode: SLEEP (deep sleep between scans)");
            break;
    }
    
    Serial.printf("[PWR] Mode changed: %d -> %d\n", prevMode, mode);
}

const char* PowerManager::getModeName() const {
    switch (currentMode) {
        case POWER_FULL: return "Full";
        case POWER_BALANCED: return "Balanced";
        case POWER_LOW: return "Low Power";
        case POWER_SLEEP: return "Deep Sleep";
        default: return "Unknown";
    }
}

float PowerManager::getBatteryVoltage() {
    if (!batteryMonitoring) return 0.0f;
    
    // Multi-sample averaging for stability
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        sum += adc1_get_raw(ADC1_CHANNEL_0);
    }
    uint32_t rawADC = sum / 16;
    
    // Convert ADC reading to voltage
    // ESP32 ADC: 0-4095 maps to 0-3.3V (with 11dB attenuation)
    float measuredVoltage = (rawADC / 4095.0f) * 3.3f;
    batteryVoltage = measuredVoltage * VBAT_DIVIDER_RATIO;
    
    // Calculate percentage
    if (batteryVoltage >= VBAT_FULL) {
        batteryPercent = 100;
    } else if (batteryVoltage <= VBAT_EMPTY) {
        batteryPercent = 0;
    } else {
        batteryPercent = (uint8_t)(((batteryVoltage - VBAT_EMPTY) / (VBAT_FULL - VBAT_EMPTY)) * 100.0f);
    }
    
    return batteryVoltage;
}

uint8_t PowerManager::getBatteryPercent() {
    return batteryPercent;
}

bool PowerManager::isBatteryLow() {
    return batteryPercent < 15;
}

bool PowerManager::isBatteryCritical() {
    return batteryPercent < 5;
}

void PowerManager::configureSleep(uint32_t sleepSeconds, uint8_t scansPerWake) {
    sleepDurationUs = sleepSeconds * 1000000ULL;
    wakeScansBeforeSleep = scansPerWake;
    Serial.printf("[PWR] Sleep config: %us, %d scans/wake\n", sleepSeconds, scansPerWake);
}

void PowerManager::enterDeepSleep() {
    Serial.printf("[PWR] Entering deep sleep for %u seconds...\n", sleepDurationUs / 1000000);
    Serial.flush();
    
    // Configure wake source: timer
    esp_sleep_enable_timer_wakeup(sleepDurationUs);
    
    // Can also wake on GPIO (e.g., button press)
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // Wake on GPIO0 LOW
    
    esp_deep_sleep_start();
    // Never reaches here — ESP32 resets on wake
}

void PowerManager::enterLightSleep(uint32_t durationMs) {
    Serial.printf("[PWR] Light sleep: %u ms\n", durationMs);
    Serial.flush();
    
    esp_sleep_enable_timer_wakeup(durationMs * 1000ULL);
    esp_light_sleep_start();
    
    Serial.println("[PWR] Woke from light sleep");
}

bool PowerManager::isWakeFromSleep() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    return (cause == ESP_SLEEP_WAKEUP_TIMER || 
            cause == ESP_SLEEP_WAKEUP_EXT0 ||
            cause == ESP_SLEEP_WAKEUP_EXT1);
}

void PowerManager::setCPUFrequency(uint32_t mhz) {
    if (mhz != 80 && mhz != 160 && mhz != 240) {
        mhz = 240;  // Fallback to default
    }
    setCpuFrequencyMhz(mhz);
    Serial.printf("[PWR] CPU: %u MHz\n", getCpuFrequencyMhz());
}

void PowerManager::disableBluetoothPower() {
    esp_bt_controller_disable();
    Serial.println("[PWR] Bluetooth disabled");
}

void PowerManager::enableBluetoothPower() {
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    Serial.println("[PWR] Bluetooth enabled (BLE)");
}

void PowerManager::setWiFiPowerSave(bool enable) {
    if (enable) {
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    } else {
        esp_wifi_set_ps(WIFI_PS_NONE);
    }
}

uint32_t PowerManager::getEstimatedRuntimeMinutes() {
    if (batteryPercent == 0) return 0;
    
    // Rough estimations based on power mode and 2500mAh battery
    float currentDrawMA;
    switch (currentMode) {
        case POWER_FULL: currentDrawMA = 450; break;
        case POWER_BALANCED: currentDrawMA = 280; break;
        case POWER_LOW: currentDrawMA = 150; break;
        case POWER_SLEEP: currentDrawMA = 30; break;  // Average including sleep
        default: currentDrawMA = 400; break;
    }
    
    float remainingMAh = 2500.0f * (batteryPercent / 100.0f);
    return (uint32_t)((remainingMAh / currentDrawMA) * 60.0f);
}
