# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Remote ID Detection (FAA ANSI/CTA-2063)**: WiFi beacon and BLE scanning for drone Remote ID broadcasts, parsing UAS serial numbers, operator location, altitude, and flight data
- **Web Interface**: ESP32 Access Point with real-time WebSocket dashboard, RSSI graphs, drone detection map, and browser-based configuration
- **Data Logging**: SD card forensic logging with CSV/JSON export, automatic log rotation, and timestamp-based detection trails
- **GPS Module**: NEO-6M/7M UART integration for geolocation tracking, geofencing alerts, and drone trajectory analysis
- **Meshtastic Integration**: LoRa mesh networking for distributed detection across large areas, collaborative threat tracking, and multi-node coordination
- **Machine Learning**: TensorFlow Lite Micro edge inference for drone type classification (DJI Phantom/Mavic, FPV Racing, Military), reducing false positives
- Full RF driver implementations (CC1101, NRF24L01+, RX5808)
- MAVLink protocol parser with command injection
- CRSF/ExpressLRS protocol parser
- Countermeasure system with threat assessment
- Bilingual documentation (English/Russian)
- Hardware setup guide with BOM
- Legal compliance documentation
- Software API reference
- Wiring diagram (SVG)
- GPL-3.0 license

### Changed
- Migrated from MIT to GPL-3.0 license
- Updated README with comprehensive project information
- Refactored main.cpp with FreeRTOS task architecture for parallel processing
- Added feature flags for modular compilation

### Security
- Added compile-time flag for countermeasures
- Implemented legal warnings and disclaimers
- Documented regulatory requirements
- Remote ID scanning is passive and legal (no active transmission)

## [0.1.0] - 2026-03-28

### Added
- Initial project structure
- Basic ESP32 configuration
- Placeholder RF module support
- OLED display integration
- PlatformIO build system

[Unreleased]: https://github.com/bobberdolle1/SkySweep32/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/bobberdolle1/SkySweep32/releases/tag/v0.1.0
