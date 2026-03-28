# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.5.0] - 2026-03-28

### Added
- **Beginner Guides ("Start from Zero")**: New comprehensive assembly guides for both [English](docs/en/start_from_zero.md) and [Russian](docs/ru/start_from_zero.md) users. No soldering required, focus on modular Starter Tier assembly.
- **Universal Flash Script**: Added `flash.bat` as a one-click firmware installer for Windows users, simplifying the setup process for non-technical users.

### Localized
- **Full Russian Web Dashboard**: Translated all real-time UI elements including Uptime, RAM usage, Battery percentages, Power modes, and Threat classifications into Russian.

### Fixed
- **WebServer Config Ref**: Corrected `RuntimeConfig` reference handling in `web_server.cpp` to ensure UI actions (like calibration) persist correctly.
- **ConfigManager Cleanup**: Standardized configuration saving logic across the core architecture.

## [0.4.0] - 2026-03-28

### Added
- **Interactive Map** (Leaflet.js + OpenStreetMap): Live drone positions on dashboard map with blue markers. Operators shown as orange markers. Auto-centers on first detection
- **Multi-Band RF Scanning**: CC1101 now scans 433/868/915 MHz ISM bands in rotation. `setBand()`, `scanAllBands()`, and `spectrumScan()` methods
- **Power Management** (`power_manager.h/cpp`):
  - 4 power modes: Full (240MHz) → Balanced (160MHz) → Low (80MHz) → Deep Sleep
  - Battery voltage monitoring via ADC (voltage divider on GPIO36)
  - Auto-low-power on critical battery (<5%)
  - Deep sleep / light sleep with timer wakeup
  - CPU frequency scaling, WiFi/BLE power state control
  - Estimated runtime calculation
- **Dashboard: Battery + Power**: Battery %, voltage, power mode, and estimated runtime shown live on web dashboard
- **API: Power Mode**: `POST /api/power?mode=0-3` — remotely switch power modes
- **Runtime Config Integration**: RF scan interval now uses `configManager` value (changeable via API)
- **NRF24 Spectrum Scanner**: Hardware 125-channel 2.4 GHz spectrum sweep via RPD.
- **Signal Database (`signal_database.h/cpp`)**: Preloaded with 8 unique drone RF signatures (DJI, FPV, ArduPilot, etc.) with advanced ML-like factor matching based on bands and RSSI variance.
- **ESP-NOW Mesh (`espnow_mesh.h/cpp`)**: Free decentralized node-to-node communication sharing threats, telemetry, and heartbeats autonomously between active SkySweep nodes.
- **Alert Manager (`alert_manager.h/cpp`)**: External notifications module with non-blocking buzzer tones and LED control corresponding to dynamic threat levels.
- **3D Enclosures Documentation**: Added EN & RU guides (`docs/en/enclosures.md`) with design recommendations, optimal materials, cooling, and antenna placement strategies for 3D printed cases.
- **Auto-Calibrate UI Button**: Added `/api/calibrate` endpoint and dashboard GUI button to automatically zero background RF noise and adjust threat thresholds.

## [0.3.1] - 2026-03-28

### Added
- **OTA Firmware Updates**: Upload firmware via `POST /api/ota` endpoint
- **Runtime Configuration**: JSON config stored on SPIFFS (`config_manager.h/cpp`)
  - WiFi, RSSI thresholds, scan intervals, LoRa settings editable without recompile
  - REST API: `GET/POST /api/config`, `POST /api/config/reset`
  - Partial updates supported (only send changed fields)
- **Watchdog Timer**: 30-second hardware watchdog prevents system hangs
- **`#ifdef` Guards on All Drivers**: CC1101, NRF24, RX5808 drivers wrapped in module guards for clean conditional compilation
- **Software Guide**: `docs/en/software.md` and `docs/ru/software.md` — Quick start, API reference, OTA, troubleshooting
- **Contributing Guide**: `CONTRIBUTING.md` — Code conventions, priority areas

### Fixed
- **`config.h` RF_MODULE_COUNT Macro**: Replaced broken `defined()` in `#define` arithmetic with proper `#ifdef` chain
- **Duplicate `#define` Conflict**: Removed duplicated RSSI threshold defines from `countermeasures.cpp` (now in `config.h`)

## [0.3.0] - 2026-03-28

### Added
- **Modular Tier System**: Three hardware tiers (Starter $15 / Hunter $35 / Sentinel $60+)
- **Central `config.h`**: All pins, modules, and settings in one file
- **SPI Bus Manager**: FreeRTOS mutex for safe multi-device SPI access (`spi_manager.h/cpp`)
- **FreeRTOS Multi-Task Architecture**: 7+ tasks across dual ESP32 cores
- **Full Web Dashboard**: Dark-themed HTML dashboard with real-time WebSocket updates, RSSI graphs, drone detection list, system status, and module badges
- **Rule-Based ML Classifier**: Classifies drones by protocol detection (MAVLink/CRSF), frequency band patterns, and RSSI variance analysis
- **Protocol Integration**: MAVLink and CRSF parsers connected to RF scanning pipeline
- **Modular Upgrade Guide**: `docs/en/modules.md` and `docs/ru/modules.md` with BOM, wiring, and upgrade paths

### Fixed
- **CRITICAL: Pin Conflicts Resolved**: GPS (GPIO 16/17) no longer conflicts with RF CS pins; LoRa (GPIO 26/33/32/25) no longer conflicts with CC1101/NRF24
- **CRITICAL: `web_server.cpp` Implemented**: Was completely empty (0 bytes), now full dashboard
- **WiFi Mode Conflict**: Remote ID no longer calls `WiFi.mode(WIFI_STA)` — web server manages WiFi in `WIFI_AP_STA` mode
- **Remote ID Memory Leak**: Replaced unbounded `std::vector` with fixed-size array (`MAX_DETECTED_DRONES = 20`)
- **Acoustic Detector I2S Mismatch**: Changed from `I2S_BITS_PER_SAMPLE_32BIT` to `16BIT` to match `int16_t` buffer
- **Data Logger File Handle Leaks**: All `File` objects now properly closed after iteration
- **Data Logger `deleteOldestLog()`**: Now builds full path with `LOG_DIR` prefix
- **ML Classifier Stub Removed**: No longer returns hardcoded values

### Changed
- **main.cpp**: Complete rewrite from monolithic `loop()` to FreeRTOS task-based architecture
- **All modules**: Wrapped in `#ifdef MODULE_*` guards for conditional compilation
- **All headers**: Use `config.h` instead of scattered local `#define` statements
- **Countermeasures**: Use SPI mutex, threshold values from `config.h`
- **Meshtastic Client**: Packet buffer limited to 50 entries to prevent memory issues

### Security
- Countermeasures require both `ENABLE_COUNTERMEASURES` build flag AND runtime `armSystem(true)` call
- WebSocket connections limited to `MAX_WEBSOCKET_CLIENTS` (4) to prevent crash

## [0.2.0] - 2026-03-28

### Added
- Remote ID Detection (FAA ANSI/CTA-2063)
- Web Interface (placeholder)
- Data Logging to SD card
- GPS Module integration
- Meshtastic LoRa mesh networking
- Machine Learning placeholder
- Full RF driver implementations
- MAVLink/CRSF protocol parsers
- Countermeasure system
- Bilingual documentation (EN/RU)
- GPL-3.0 license

## [0.1.0] - 2026-03-28

### Added
- Initial project structure
- Basic ESP32 configuration
- Placeholder RF module support
- OLED display integration
- PlatformIO build system

[0.3.0]: https://github.com/bobberdolle1/SkySweep32/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/bobberdolle1/SkySweep32/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/bobberdolle1/SkySweep32/releases/tag/v0.1.0
