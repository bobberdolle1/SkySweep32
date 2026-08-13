#include "power_manager.h"
#ifdef MODULE_BATTERY_GAUGE
#include <Wire.h>
#endif
#include <esp_wifi.h>
#include <esp_bt.h>

PowerManager powerManager;

#ifdef MODULE_BATTERY_GAUGE
namespace {
constexpr uint8_t kMax17048Address = 0x36;
constexpr uint8_t kMax17048VCellRegister = 0x02;
constexpr uint8_t kMax17048SocRegister = 0x04;

bool readMax17048Register(uint8_t reg, uint16_t& value) {
    Wire.beginTransmission(kMax17048Address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    if (Wire.requestFrom(kMax17048Address, static_cast<uint8_t>(2)) != 2) {
        return false;
    }
    value = static_cast<uint16_t>(Wire.read()) << 8;
    value |= Wire.read();
    return true;
}
}  // namespace
#endif

PowerManager::PowerManager()
    : currentMode(POWER_FULL),
      batteryVoltage(0.0f),
      batteryPercent(100),
      batteryMonitoring(false),
      lastBatteryRead(0) {}

void PowerManager::begin() {
    
    #ifdef MODULE_BATTERY_GAUGE
    Wire.beginTransmission(kMax17048Address);
    batteryMonitoring = Wire.endTransmission() == 0;
    if (!batteryMonitoring) {
        Serial.println("[PWR] MAX17048 fuel gauge not detected");
    }
    #else
    pinMode(PIN_VBAT_ADC, INPUT);
    batteryMonitoring = true;
    #endif

    // Initial battery read.
    getBatteryVoltage();
    
    setCPUFrequency(240);
    Serial.printf("[PWR] Mode: %s\n", getModeName());
}

void PowerManager::update() {
    // Read battery every 10 seconds
    if (millis() - lastBatteryRead > 10000) {
        getBatteryVoltage();
        lastBatteryRead = millis();
        
        // Critical battery selects the lowest actually implemented policy.
        if (isBatteryCritical() && currentMode == POWER_FULL) {
            Serial.println("[PWR] Battery critical; switching to BALANCED mode");
            setMode(POWER_BALANCED);
        }
    }
}

void PowerManager::setMode(PowerMode mode) {
    if (mode == currentMode) {
        setCPUFrequency(mode == POWER_BALANCED ? 160 : 240);
        applyWiFiPolicy();
        return;
    }

    const PowerMode previous = currentMode;
    currentMode = mode;
    switch (mode) {
        case POWER_FULL:
            setCPUFrequency(240);
            setWiFiPowerSave(false);
            Serial.println("[PWR] Mode: FULL (240 MHz, Wi-Fi power save disabled)");
            break;
        case POWER_BALANCED:
            setCPUFrequency(160);
            setWiFiPowerSave(true);
            Serial.println("[PWR] Mode: BALANCED (160 MHz, Wi-Fi modem power save)");
            break;
        default:
            currentMode = POWER_FULL;
            setCPUFrequency(240);
            setWiFiPowerSave(false);
            Serial.println("[PWR] Invalid mode; restored FULL");
            break;
    }
    Serial.printf("[PWR] Mode changed: %d -> %d\n", previous, currentMode);
}

const char* PowerManager::getModeName() const {
    switch (currentMode) {
        case POWER_FULL: return "Full";
        case POWER_BALANCED: return "Balanced";
        default: return "Unknown";
    }
}

float PowerManager::getBatteryVoltage() {
    if (!batteryMonitoring) return 0.0f;

    #ifdef MODULE_BATTERY_GAUGE
    uint16_t rawVCell = 0;
    uint16_t rawSoc = 0;
    if (!readMax17048Register(kMax17048VCellRegister, rawVCell) ||
        !readMax17048Register(kMax17048SocRegister, rawSoc)) {
        return batteryVoltage;
    }

    // MAX17048 VCELL register LSB is 78.125 uV; its low nibble is always zero.
    batteryVoltage = static_cast<float>(rawVCell) * 0.000078125f;
    const float stateOfCharge = static_cast<float>(rawSoc) / 256.0f;
    batteryPercent = static_cast<uint8_t>(
        constrain(stateOfCharge, 0.0f, 100.0f) + 0.5f);
    #else
    uint32_t sum = 0;
    for (int sample = 0; sample < 16; ++sample) {
        sum += analogRead(PIN_VBAT_ADC);
    }
    const uint32_t rawADC = sum / 16;
    const float measuredVoltage = (rawADC / 4095.0f) * 3.3f;
    batteryVoltage = measuredVoltage * VBAT_DIVIDER_RATIO;
    if (batteryVoltage >= VBAT_FULL) {
        batteryPercent = 100;
    } else if (batteryVoltage <= VBAT_EMPTY) {
        batteryPercent = 0;
    } else {
        batteryPercent = static_cast<uint8_t>(
            ((batteryVoltage - VBAT_EMPTY) / (VBAT_FULL - VBAT_EMPTY)) *
            100.0f);
    }
    #endif

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


void PowerManager::setCPUFrequency(uint32_t mhz) {
    if (mhz != 160 && mhz != 240) {
        mhz = 240;
    }
    setCpuFrequencyMhz(mhz);
    Serial.printf("[PWR] CPU: %u MHz\n", getCpuFrequencyMhz());
}


void PowerManager::applyWiFiPolicy() {
    setWiFiPowerSave(currentMode == POWER_BALANCED);
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
    const float currentDrawMA = currentMode == POWER_BALANCED ? 280.0f : 450.0f;
    const float remainingMAh = 2500.0f * (batteryPercent / 100.0f);
    return static_cast<uint32_t>((remainingMAh / currentDrawMA) * 60.0f);
}
