# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
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

### Security
- Added compile-time flag for countermeasures
- Implemented legal warnings and disclaimers
- Documented regulatory requirements

## [0.1.0] - 2026-03-28

### Added
- Initial project structure
- Basic ESP32 configuration
- Placeholder RF module support
- OLED display integration
- PlatformIO build system

[Unreleased]: https://github.com/bobberdolle1/SkySweep32/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/bobberdolle1/SkySweep32/releases/tag/v0.1.0
