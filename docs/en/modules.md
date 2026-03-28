# SkySweep32 Modular Upgrade Guide

## 📦 Hardware Tiers

SkySweep32 is designed to be **budget-friendly and modular**. Start with the base configuration and upgrade as needed.

### 🟢 Tier 1: "Starter" (~$15-20)

**What you get**: Basic 2.4 GHz monitoring with web dashboard

| Component | Model | Price | Purpose |
|-----------|-------|-------|---------|
| Microcontroller | ESP32 DevKit V1 | ~$5-8 | Main processor + WiFi + BLE |
| Display | OLED 128x64 (SSD1306) I2C | ~$3-4 | Visual feedback |
| RF Module | NRF24L01+ | ~$2-3 | 2.4 GHz monitoring |
| **Total** | | **~$15** | |

**Capabilities**:
- ✅ 2.4 GHz band monitoring (WiFi, DJI, ExpressLRS)
- ✅ Remote ID detection via BLE (free — uses ESP32 built-in Bluetooth)
- ✅ Web dashboard via WiFi (free — uses ESP32 built-in WiFi)
- ✅ OLED display with real-time RSSI
- ✅ 5-level threat assessment
- ❌ No 900 MHz / 5.8 GHz coverage
- ❌ No GPS / logging / mesh networking

**Wiring**:
```
ESP32 → NRF24L01+:
  GPIO 23 (MOSI) → MOSI
  GPIO 19 (MISO) → MISO
  GPIO 18 (SCK)  → SCK
  GPIO 15 (CS)   → CSN
  GPIO 2  (CE)   → CE
  3.3V           → VCC
  GND            → GND

ESP32 → OLED:
  GPIO 21 (SDA)  → SDA
  GPIO 22 (SCL)  → SCL
  3.3V           → VCC
  GND            → GND
```

**Build**: 
```bash
pio run  # Default: TIER_STANDARD
# For Base tier only:
# Add to platformio.ini: build_flags = -DTIER_BASE
```

---

### 🟡 Tier 2: "Hunter" (~$35-45)

**Upgrade from Starter**: Add 900 MHz and 5.8 GHz modules

| Additional Component | Model | Price | Purpose |
|---------------------|-------|-------|---------|
| RF Module (900 MHz) | CC1101 | ~$5-7 | ISM band monitoring |
| RF Module (5.8 GHz) | RX5808 | ~$8-12 | FPV video detection |
| **Upgrade Cost** | | **~$15-20** | |
| **Total (with Starter)** | | **~$35** | |

**New Capabilities**:
- ✅ Triple-band monitoring: 900 MHz + 2.4 GHz + 5.8 GHz
- ✅ Protocol detection: MAVLink, CRSF/ExpressLRS
- ✅ AI drone classification (rule-based: DJI, FPV, Military)
- ✅ Analog video signal detection

**Additional Wiring**:
```
ESP32 → CC1101:
  GPIO 23 (MOSI) → MOSI  (shared SPI)
  GPIO 19 (MISO) → MISO  (shared SPI)
  GPIO 18 (SCK)  → SCK   (shared SPI)
  GPIO 5  (CS)   → CS
  3.3V           → VCC
  GND            → GND

ESP32 → RX5808:
  GPIO 23 (MOSI) → MOSI  (shared SPI)
  GPIO 19 (MISO) → MISO  (shared SPI)
  GPIO 18 (SCK)  → SCK   (shared SPI)
  GPIO 13 (CS)   → CS
  GPIO 34 (RSSI) → RSSI (analog output)
  3.3V           → VCC
  GND            → GND
```

---

### 🔴 Tier 3: "Sentinel" (~$60-80)

**Upgrade from Hunter**: Add GPS, SD card, LoRa mesh

| Additional Component | Model | Price | Purpose |
|---------------------|-------|-------|---------|
| GPS Module | NEO-6M / NEO-7M | ~$5-8 | Geolocation |
| SD Card Module | MicroSD SPI | ~$2-3 | Forensic logging |
| LoRa Module | SX1276 (868/915 MHz) | ~$8-12 | Mesh networking |
| SD Card | 4-32 GB MicroSD | ~$3-5 | Storage |
| **Upgrade Cost** | | **~$20-30** | |
| **Total (with Hunter)** | | **~$60** | |

**New Capabilities**:
- ✅ GPS geolocation of detection events
- ✅ Geofence alerts
- ✅ SD card forensic logging (CSV/JSON export)
- ✅ LoRa mesh networking (multi-node detection)
- ✅ Collaborative threat tracking

**Additional Wiring**:
```
ESP32 → GPS (NEO-6M):
  GPIO 16 (RX) → TX
  GPIO 17 (TX) → RX
  3.3V         → VCC
  GND          → GND

ESP32 → SD Card Module:
  GPIO 23 (MOSI) → MOSI  (shared SPI)
  GPIO 19 (MISO) → MISO  (shared SPI)
  GPIO 18 (SCK)  → SCK   (shared SPI)
  GPIO 27 (CS)   → CS
  3.3V           → VCC
  GND            → GND

ESP32 → SX1276 LoRa:
  GPIO 23 (MOSI) → MOSI  (shared SPI)
  GPIO 19 (MISO) → MISO  (shared SPI)
  GPIO 18 (SCK)  → SCK   (shared SPI)
  GPIO 26 (CS)   → NSS
  GPIO 33 (DIO0) → DIO0
  GPIO 32 (DIO1) → DIO1
  GPIO 25 (RST)  → RESET
  3.3V           → VCC
  GND            → GND
```

---

## 🎛️ Optional Modules (Any Tier)

### 🎤 Acoustic Detection (~$5)

Add to any tier for rotor sound detection.

| Component | Model | Price |
|-----------|-------|-------|
| MEMS Microphone | ICS-43434 (I2S) | ~$4-5 |

**Wiring**:
```
ESP32 → ICS-43434:
  GPIO 14 (BCLK) → BCLK
  GPIO 12 (WS)   → WS
  GPIO 35 (DIN)  → DOUT
  3.3V            → VCC
  GND             → GND
```

**Enable**: Add `-DMODULE_ACOUSTIC` to `platformio.ini` build_flags

---

## 🔧 Configuration

All configuration is centralized in `src/config.h`:

```cpp
// Select tier (or use build_flags):
#define TIER_STANDARD  // Change to TIER_BASE or TIER_PRO

// Individual module overrides:
#define MODULE_ACOUSTIC  // Add acoustic to any tier
```

### Using platformio.ini:
```ini
; Base tier
build_flags = -DTIER_BASE

; Standard tier with acoustic
build_flags = -DTIER_STANDARD -DMODULE_ACOUSTIC

; Pro tier with countermeasures (LEGAL AUTH REQUIRED!)
build_flags = -DTIER_PRO -DENABLE_COUNTERMEASURES

; Custom: Base + GPS only
build_flags = -DTIER_BASE -DMODULE_GPS
```

---

## 📍 Complete Pin Map (All Tiers)

| Pin | Function | Tier |
|-----|----------|------|
| GPIO 2 | NRF24L01+ CE | Base+ |
| GPIO 5 | CC1101 CS | Standard+ |
| GPIO 12 | I2S WS (Acoustic) | Optional |
| GPIO 13 | RX5808 CS | Standard+ |
| GPIO 14 | I2S BCLK (Acoustic) | Optional |
| GPIO 15 | NRF24L01+ CS | Base+ |
| GPIO 16 | GPS RX (UART2) | Pro |
| GPIO 17 | GPS TX (UART2) | Pro |
| GPIO 18 | SPI SCK | Base+ |
| GPIO 19 | SPI MISO | Base+ |
| GPIO 21 | I2C SDA (OLED) | Base+ |
| GPIO 22 | I2C SCL (OLED) | Base+ |
| GPIO 23 | SPI MOSI | Base+ |
| GPIO 25 | LoRa RESET | Pro |
| GPIO 26 | LoRa CS | Pro |
| GPIO 27 | SD Card CS | Pro |
| GPIO 32 | LoRa DIO1 | Pro |
| GPIO 33 | LoRa DIO0 | Pro |
| GPIO 34 | RX5808 RSSI (ADC) | Standard+ |
| GPIO 35 | I2S DIN (Acoustic) | Optional |

**Total GPIO used**: Base=7, Standard=10, Pro=16, Full=18
