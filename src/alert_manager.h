#ifndef ALERT_MANAGER_H
#define ALERT_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Buzzer pin (optional — connect passive buzzer to this GPIO)
#ifndef PIN_BUZZER
#define PIN_BUZZER          4     // Available GPIO, change if needed
#endif

// LED pin (optional — external alert LED)
#ifndef PIN_ALERT_LED
#define PIN_ALERT_LED       2     // ESP32 built-in LED (if available)
#endif

// Alert types
enum AlertType {
    ALERT_NONE = 0,
    ALERT_INFO,           // Short beep, LED blink
    ALERT_DRONE_DETECTED, // Double beep, fast blink
    ALERT_THREAT_HIGH,    // Rapid beeps, solid LED
    ALERT_THREAT_CRITICAL,// Continuous alarm, flashing LED
    ALERT_BATTERY_LOW,    // Slow triple beep
    ALERT_MESH_ALERT      // Different pattern for remote alerts
};

// Alert tones (frequency in Hz)
#define TONE_INFO       1000
#define TONE_DETECT     2000
#define TONE_HIGH       2500
#define TONE_CRITICAL   3000
#define TONE_LOW_BAT    800
#define TONE_MESH       1500

class AlertManager {
private:
    bool buzzerEnabled;
    bool ledEnabled;
    bool isMuted;
    
    AlertType currentAlert;
    uint32_t alertStartTime;
    uint32_t lastToggle;
    bool ledState;
    
    // Non-blocking tone generation
    uint32_t toneEndTime;
    uint8_t  beepCount;
    uint8_t  beepsRemaining;
    uint16_t beepFreq;
    uint16_t beepDurationMs;
    uint16_t beepGapMs;
    
    void startPattern(uint16_t freq, uint8_t count, uint16_t durMs, uint16_t gapMs);
    
public:
    AlertManager();
    
    void begin(bool enableBuzzer = true, bool enableLed = true);
    void update();  // Call from loop/task — handles non-blocking patterns
    
    // Trigger alerts
    void alert(AlertType type);
    void clearAlert();
    
    // Control
    void mute(bool m) { isMuted = m; }
    bool isMutedState() const { return isMuted; }
    void setBuzzerEnabled(bool e) { buzzerEnabled = e; }
    void setLedEnabled(bool e) { ledEnabled = e; }
    
    // Direct tone (blocking)
    void playTone(uint16_t freq, uint16_t durationMs);
    void beep(uint8_t count = 1, uint16_t freq = 2000);
    
    AlertType getCurrentAlert() const { return currentAlert; }
};

extern AlertManager alertManager;

#endif // ALERT_MANAGER_H
