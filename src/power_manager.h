#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include <esp_sleep.h>
#include <esp_pm.h>
#include <driver/adc.h>

// Power modes
enum PowerMode {
    POWER_FULL = 0,       // All modules active, max scan rate
    POWER_BALANCED = 1,   // Reduced scan rate, WiFi power save
    POWER_LOW = 2,        // Minimal scanning, OLED off
    POWER_SLEEP = 3       // Deep sleep between scan cycles
};

// Battery monitoring (optional — connect battery voltage divider to GPIO36)
#define PIN_VBAT_ADC        36    // ADC1_CH0, input-only
#define VBAT_DIVIDER_RATIO  2.0f  // Voltage divider: 100k/100k
#define VBAT_FULL           4.2f
#define VBAT_EMPTY          3.3f

class PowerManager {
private:
    PowerMode currentMode;
    float batteryVoltage;
    uint8_t batteryPercent;
    bool batteryMonitoring;
    uint32_t lastBatteryRead;
    
    // Deep sleep configuration
    uint32_t sleepDurationUs;     // Microseconds between wake cycles
    uint8_t  wakeScansBeforeSleep; // Number of scans before going back to sleep
    
public:
    PowerManager();
    
    void begin();
    void update();
    
    // Power mode control
    void setMode(PowerMode mode);
    PowerMode getMode() const { return currentMode; }
    const char* getModeName() const;
    
    // Battery monitoring
    float getBatteryVoltage();
    uint8_t getBatteryPercent();
    bool isBatteryLow();  // < 15%
    bool isBatteryCritical(); // < 5%
    
    // Deep sleep
    void configureSleep(uint32_t sleepSeconds, uint8_t scansPerWake = 3);
    void enterDeepSleep();
    void enterLightSleep(uint32_t durationMs);
    bool isWakeFromSleep();
    
    // CPU frequency scaling
    void setCPUFrequency(uint32_t mhz);  // 80, 160, or 240 MHz
    
    // Peripheral power control
    void disableBluetoothPower();
    void enableBluetoothPower();
    void setWiFiPowerSave(bool enable);
    
    // Power stats for dashboard
    uint32_t getEstimatedRuntimeMinutes();
};

extern PowerManager powerManager;

#endif // POWER_MANAGER_H
