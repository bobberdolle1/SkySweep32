#include "alert_manager.h"

AlertManager alertManager;

AlertManager::AlertManager()
    : buzzerEnabled(false), ledEnabled(false), isMuted(false),
      currentAlert(ALERT_NONE), alertStartTime(0), lastToggle(0),
      ledState(false), toneEndTime(0), beepCount(0), beepsRemaining(0),
      beepFreq(0), beepDurationMs(0), beepGapMs(0) {}

void AlertManager::begin(bool enableBuzzer, bool enableLed) {
    buzzerEnabled = enableBuzzer;
    ledEnabled = enableLed;
    
    if (buzzerEnabled) {
        pinMode(PIN_BUZZER, OUTPUT);
        digitalWrite(PIN_BUZZER, LOW);
        Serial.printf("[ALERT] Buzzer enabled on GPIO%d\n", PIN_BUZZER);
    }
    
    if (ledEnabled) {
        pinMode(PIN_ALERT_LED, OUTPUT);
        digitalWrite(PIN_ALERT_LED, LOW);
        Serial.printf("[ALERT] LED enabled on GPIO%d\n", PIN_ALERT_LED);
    }
    
    // Startup chirp
    if (buzzerEnabled && !isMuted) {
        playTone(1000, 50);
        delay(50);
        playTone(2000, 50);
    }
}

void AlertManager::update() {
    // Handle non-blocking beep patterns
    if (beepsRemaining > 0) {
        uint32_t now = millis();
        
        if (toneEndTime > 0 && now >= toneEndTime) {
            // Tone just ended
            noTone(PIN_BUZZER);
            toneEndTime = 0;
            
            beepsRemaining--;
            if (beepsRemaining > 0) {
                // Schedule next beep after gap
                lastToggle = now;
            }
        } else if (toneEndTime == 0 && now - lastToggle >= beepGapMs) {
            // Gap ended, play next beep
            if (buzzerEnabled && !isMuted) {
                tone(PIN_BUZZER, beepFreq, beepDurationMs);
            }
            toneEndTime = now + beepDurationMs;
        }
    }
    
    // Handle LED patterns
    if (ledEnabled && currentAlert != ALERT_NONE) {
        uint32_t now = millis();
        uint16_t blinkRate;
        
        switch (currentAlert) {
            case ALERT_INFO:           blinkRate = 500; break;
            case ALERT_DRONE_DETECTED: blinkRate = 250; break;
            case ALERT_THREAT_HIGH:    blinkRate = 100; break;
            case ALERT_THREAT_CRITICAL:blinkRate = 50;  break;
            case ALERT_BATTERY_LOW:    blinkRate = 1000; break;
            case ALERT_MESH_ALERT:     blinkRate = 300; break;
            default:                   blinkRate = 500; break;
        }
        
        if (now - lastToggle >= blinkRate) {
            ledState = !ledState;
            digitalWrite(PIN_ALERT_LED, ledState ? HIGH : LOW);
            lastToggle = now;
        }
        
        // Auto-clear alerts after timeout
        if (currentAlert != ALERT_THREAT_CRITICAL && 
            now - alertStartTime > 10000) {
            clearAlert();
        }
    }
}

void AlertManager::alert(AlertType type) {
    currentAlert = type;
    alertStartTime = millis();
    
    switch (type) {
        case ALERT_INFO:
            startPattern(TONE_INFO, 1, 100, 0);
            break;
        case ALERT_DRONE_DETECTED:
            startPattern(TONE_DETECT, 2, 80, 80);
            break;
        case ALERT_THREAT_HIGH:
            startPattern(TONE_HIGH, 3, 100, 50);
            break;
        case ALERT_THREAT_CRITICAL:
            startPattern(TONE_CRITICAL, 5, 150, 50);
            break;
        case ALERT_BATTERY_LOW:
            startPattern(TONE_LOW_BAT, 3, 200, 300);
            break;
        case ALERT_MESH_ALERT:
            startPattern(TONE_MESH, 2, 150, 100);
            break;
        default:
            break;
    }
}

void AlertManager::clearAlert() {
    currentAlert = ALERT_NONE;
    beepsRemaining = 0;
    toneEndTime = 0;
    
    if (buzzerEnabled) {
        noTone(PIN_BUZZER);
    }
    if (ledEnabled) {
        digitalWrite(PIN_ALERT_LED, LOW);
        ledState = false;
    }
}

void AlertManager::startPattern(uint16_t freq, uint8_t count, uint16_t durMs, uint16_t gapMs) {
    beepFreq = freq;
    beepCount = count;
    beepsRemaining = count;
    beepDurationMs = durMs;
    beepGapMs = gapMs;
    toneEndTime = 0;
    lastToggle = millis() - gapMs;  // Trigger first beep immediately
}

void AlertManager::playTone(uint16_t freq, uint16_t durationMs) {
    if (!buzzerEnabled || isMuted) return;
    tone(PIN_BUZZER, freq, durationMs);
}

void AlertManager::beep(uint8_t count, uint16_t freq) {
    if (!buzzerEnabled || isMuted) return;
    
    for (uint8_t i = 0; i < count; i++) {
        tone(PIN_BUZZER, freq, 80);
        delay(120);
    }
    noTone(PIN_BUZZER);
}
