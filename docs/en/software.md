# Software Configuration Guide

## Quick Start

### 1. Install PlatformIO

Download and install [PlatformIO IDE](https://platformio.org/install/ide) (VS Code extension recommended).

### 2. Clone Repository

```bash
git clone https://github.com/bobberdolle1/SkySweep32.git
cd SkySweep32
```

### 3. Select Your Tier

Edit `platformio.ini` and set `build_flags` for your hardware:

```ini
# Base (~$15): ESP32 + OLED + NRF24L01+
build_flags = -DTIER_BASE

# Standard (~$35): + CC1101 + RX5808 (DEFAULT)
build_flags = -DTIER_STANDARD

# Pro (~$60+): + GPS + SD Card + LoRa
build_flags = -DTIER_PRO

# Custom: any tier + optional modules
build_flags = -DTIER_STANDARD -DMODULE_ACOUSTIC
```

### 4. Build & Upload

```bash
pio run -t upload
```

### 5. Access Dashboard

1. Connect to WiFi network: **SkySweep32** (password: `skysweep32`)
2. Open browser: `http://192.168.4.1`

---

## Runtime Configuration

SkySweep32 stores settings in a JSON file on SPIFFS (internal flash filesystem). Changes take effect immediately without recompiling.

### Via Web Dashboard

Settings can be sent via the REST API:

```bash
# Get current configuration
curl http://192.168.4.1/api/config

# Update settings (partial updates supported)
curl -X POST http://192.168.4.1/api/config \
  -H "Content-Type: application/json" \
  -d '{"thresholds":{"low":50,"medium":65},"rfScanMs":200}'

# Reset to defaults
curl -X POST http://192.168.4.1/api/config/reset
```

### Configuration Fields

| Field | Default | Description |
|-------|---------|-------------|
| `wifi.ssid` | "SkySweep32" | WiFi network name |
| `wifi.password` | "skysweep32" | WiFi password |
| `wifi.apMode` | true | AP mode (true) or Station mode (false) |
| `wifi.channel` | 6 | WiFi channel (1-13) |
| `thresholds.low` | 45 | RSSI value for LOW threat |
| `thresholds.medium` | 60 | RSSI value for MEDIUM threat |
| `thresholds.high` | 75 | RSSI value for HIGH threat |
| `thresholds.critical` | 85 | RSSI value for CRITICAL threat |
| `rfScanMs` | 100 | RF polling interval (ms) |
| `displayMs` | 500 | OLED refresh rate (ms) |
| `bleScanMs` | 5000 | BLE Remote ID scan interval (ms) |
| `lora.freq` | 915.0 | LoRa frequency (MHz) |
| `lora.txPower` | 20 | LoRa transmit power (dBm) |
| `logLevel` | 1 | 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR |

---

## OTA Firmware Updates

SkySweep32 supports Over-The-Air firmware updates via the web dashboard.

### Upload via curl

```bash
# Build firmware
pio run

# Upload OTA
curl -X POST http://192.168.4.1/api/ota \
  -F "firmware=@.pio/build/esp32dev/firmware.bin"
```

The device will automatically reboot after a successful update.

### Upload via Python

```python
import requests
with open('.pio/build/esp32dev/firmware.bin', 'rb') as f:
    r = requests.post('http://192.168.4.1/api/ota', files={'firmware': f})
    print(r.json())
```

---

## REST API Reference

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Web dashboard |
| `/api/status` | GET | System status (version, uptime, heap, tier) |
| `/api/config` | GET | Current runtime configuration |
| `/api/config` | POST | Update configuration (JSON body) |
| `/api/config/reset` | POST | Reset configuration to defaults |
| `/api/ota` | POST | Upload firmware binary (multipart) |
| `/api/power` | POST | Set power mode (`?mode=0-3`: Full/Balanced/Low/Sleep) |
| `/ws` | WebSocket | Real-time data stream |

### WebSocket Message Types

```json
// RF data update
{"type":"rf","module":"CC1101","rssi":72,"active":true}

// Threat assessment
{"type":"threat","level":"HIGH","protocol":"DJI OcuSync"}

// Drone detection (Remote ID) — also plots on map
{"type":"drone","id":"UAS-12345","lat":55.7558,"lon":37.6173,"alt":120.5}

// System status (periodic) — includes battery data
{"type":"status","uptime":3600,"heap":285000,"clients":2,
 "batV":3.92,"batPct":78,"pwrMode":"Full","estMin":420,
 "modules":[...]}
```

---

## Serial Commands

The serial monitor (`115200 baud`) provides real-time logging:

```
╔══════════════════════════════════════╗
║   SkySweep32 Passive Drone Detector  ║
║   Version 0.3.0    Mar 28 2026       ║
╠══════════════════════════════════════╣
║   Tier: 🟡 STANDARD (Hunter)         ║
╚══════════════════════════════════════╝

[INIT] Configuration manager...
[INIT] Watchdog timer: 30s
[INIT] CC1101 (900 MHz)...
[CC1101] Chip version: 0x14
[CC1101] Frequency set to 915000000 Hz
[INIT] NRF24L01+ (2.4 GHz)...
[NRF24] Initialized successfully
[INIT] Web Server...
[WEB] AP started: SkySweep32 (IP: 192.168.4.1)
[TASK] RF Scanning started (Core 0)
[TASK] Display update started (Core 1)
[TASK] Web server started (Core 1)
```

---

## Build Flags Reference

| Flag | Description |
|------|-------------|
| `-DTIER_BASE` | Base tier (NRF24 + OLED + Web + BLE) |
| `-DTIER_STANDARD` | Standard tier (+ CC1101 + RX5808) |
| `-DTIER_PRO` | Pro tier (+ GPS + SD + LoRa) |
| `-DMODULE_ACOUSTIC` | Enable acoustic detection |
| `-DENABLE_COUNTERMEASURES` | Enable active countermeasures ⚠️ |

---

## Troubleshooting

### Build Errors

| Error | Solution |
|-------|----------|
| `fatal error: U8g2lib.h` | Run `pio lib install "olikraus/U8g2"` |
| `fatal error: ESPAsyncWebServer.h` | Already in `platformio.ini` lib_deps |
| `undefined reference to 'spiManager'` | Ensure `spi_manager.cpp` is in `src/` |
| `SPIFFS mount failed` | First boot formats flash automatically |

### Runtime Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| Dashboard not loading | Wrong WiFi network | Connect to "SkySweep32" |
| WebSocket disconnects | Too many clients | Max 4 simultaneous connections |
| RF module not detected | SPI wiring error | Check MOSI/MISO/SCK/CS |
| Watchdog reset | Task deadlock | Check serial log for stuck task |
| OTA fails | File too large | Use `huge_app.csv` partition scheme |
