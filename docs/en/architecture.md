# SkySweep32 Architecture Guide

## Overview

SkySweep32 uses a multi-task FreeRTOS architecture on the ESP32 dual-core processor. Each module runs in its own task, communicating through shared data structures protected by mutexes.

## Task Architecture

```
ESP32 Dual Core Architecture
═══════════════════════════════════════════════════════

Core 0 (Protocol CPU)              Core 1 (Application CPU)
┌─────────────────────┐            ┌─────────────────────┐
│ taskRFScanning      │            │ taskDisplayUpdate    │
│ Priority: 3         │            │ Priority: 1         │
│ Stack: 4096         │            │ Stack: 2048         │
│ • CC1101 polling    │            │ • OLED refresh @2Hz │
│ • NRF24 polling     │            │ • Signal bars       │
│ • RX5808 polling    │            │ • Threat indicator  │
│ • Protocol parsing  │            └─────────────────────┘
│ • Threat assessment │            ┌─────────────────────┐
│ • ML classification │            │ taskWebServer       │
└─────────────────────┘            │ Priority: 2         │
┌─────────────────────┐            │ Stack: 8192         │
│ taskRemoteID        │            │ • WiFi AP           │
│ Priority: 1         │            │ • HTTP server       │
│ Stack: 4096         │            │ • WebSocket         │
│ • BLE scanning      │            │ • Status broadcast  │
└─────────────────────┘            └─────────────────────┘
┌─────────────────────┐            ┌─────────────────────┐
│ taskLoRaMesh        │            │ taskGPS             │
│ Priority: 1         │            │ Priority: 1         │
│ Stack: 4096         │            │ Stack: 2048         │
│ • LoRa RX/TX        │            │ • UART2 polling     │
│ • Alert broadcast   │            │ • NMEA parsing      │
└─────────────────────┘            │ • Geofence check    │
┌─────────────────────┐            └─────────────────────┘
│ taskAcoustic        │
│ Priority: 2         │
│ Stack: 4096         │
│ • I2S audio read    │
│ • Goertzel filters  │
└─────────────────────┘
```

## Data Flow

```
                    ┌──────────────────────┐
                    │   RF Modules (SPI)   │
                    │  CC1101 | NRF24 | RX │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │   taskRFScanning     │
                    │  • Read RSSI         │
                    │  • Parse protocols   │
                    │  • Assess threats    │
                    └──────────┬───────────┘
                               │
          ┌────────────────────┼────────────────────┐
          │                    │                    │
┌─────────▼──────┐  ┌────────▼────────┐  ┌───────▼────────┐
│  Web Dashboard │  │   OLED Display  │  │  SD Card Log   │
│  (WebSocket)   │  │   (taskDisplay) │  │  (DataLogger)  │
└────────────────┘  └─────────────────┘  └────────────────┘
          │
          │  (on CRITICAL threat)
          │
┌─────────▼──────────┐     ┌──────────────────┐
│  LoRa Mesh Alert   │     │  BLE/WiFi Scan   │
│  (taskLoRaMesh)    │     │  (taskRemoteID)  │
└────────────────────┘     └──────────────────┘
```

## SPI Bus Sharing

All SPI devices share a single bus (GPIO 18/19/23) with separate CS pins.
Access is controlled by a FreeRTOS mutex:

```
SPIManager (spi_manager.h)
├── Mutex: xSemaphoreCreateMutex()
├── acquire(timeout) → take mutex
├── release()        → give mutex
├── selectDevice(cs) → deselect all, select one
└── deselectAll()    → all CS HIGH

SPI Devices:
├── CC1101    CS=GPIO 5   [Standard+]
├── NRF24L01+ CS=GPIO 15  [Base+]
├── RX5808    CS=GPIO 13  [Standard+]
├── LoRa      CS=GPIO 26  [Pro]
└── SD Card   CS=GPIO 27  [Pro]
```

Any task that accesses SPI MUST follow:
```cpp
if (spiManager.acquire(pdMS_TO_TICKS(100))) {
    // SPI operations here
    spiManager.release();
}
```

## Conditional Compilation

Modules are included/excluded via `config.h` feature flags:

```
TIER_BASE       → MODULE_NRF24, MODULE_OLED, MODULE_WEB_SERVER, MODULE_REMOTE_ID
TIER_STANDARD   → Base + MODULE_CC1101, MODULE_RX5808
TIER_PRO        → Standard + MODULE_GPS, MODULE_SD_CARD, MODULE_LORA
```

Code is wrapped in `#ifdef`:
```cpp
#ifdef MODULE_GPS
  gpsModule.update();
#endif
```

## Memory Usage

| Component | Estimated RAM | Notes |
|-----------|---------------|-------|
| FreeRTOS Tasks | ~35 KB | Combined stack for all tasks |
| Web Dashboard | ~15 KB | HTML in PROGMEM (Flash, not RAM) |
| RF Scan Buffers | ~2 KB | RSSI history + RX buffers |
| Remote ID Array | ~4 KB | 20 × DroneRemoteIDData |
| ArduinoJson | ~2 KB | Per serialization |
| Display Buffer | ~1 KB | 128×64 = 1024 bytes |
| **Total** | **~60 KB** | **ESP32 has 520 KB** |

Free heap after init: ~280 KB (typical with Standard tier)
