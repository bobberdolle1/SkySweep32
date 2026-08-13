#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include <esp_pm.h>
#include <driver/adc.h>

// Only modes with executed behavior are exposed by Prototype #1.
enum PowerMode {
    POWER_FULL = 0,       // 240 MHz CPU, Wi-Fi power save disabled
    POWER_BALANCED = 1    // 160 MHz CPU, Wi-Fi modem power save enabled
};

// Battery monitoring (optional single-cell input through a 100k/100k divider).
#ifndef PIN_VBAT_ADC
#define PIN_VBAT_ADC        36
#endif
#define VBAT_DIVIDER_RATIO  2.0f
#define VBAT_FULL           4.2f
#define VBAT_EMPTY          3.3f

class PowerManager {
private:
    PowerMode currentMode;
    float batteryVoltage;
    uint8_t batteryPercent;
    bool batteryMonitoring;
    uint32_t lastBatteryRead;
    
    
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
    
    // CPU and Wi-Fi policy
    void setCPUFrequency(uint32_t mhz);  // 160 or 240 MHz
    void applyWiFiPolicy();
    void setWiFiPowerSave(bool enable);
    
    // Power stats for dashboard
    uint32_t getEstimatedRuntimeMinutes();
};

extern PowerManager powerManager;

#endif // POWER_MANAGER_H
