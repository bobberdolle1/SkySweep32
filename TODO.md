# SkySweep32 TODO List

## 🔥 High Priority (Next 2 Weeks)

### Bug Fixes
- [ ] Fix web_server.cpp incomplete implementation (missing WebSocket handlers)
- [ ] Add error handling for SD card initialization failures
- [ ] Implement GPS UART conflict resolution (shares pins with other modules)
- [ ] Add mutex protection for SPI bus access (CC1101/NRF24/RX5808/SD/LoRa)
- [ ] Fix potential memory leak in RemoteIDDetector BLE scanning

### Core Features
- [ ] Complete web_server.cpp implementation (HTML dashboard generation)
- [ ] Add configuration file support (JSON on SD card)
- [ ] Implement FreeRTOS tasks for parallel module processing
- [ ] Add watchdog timer for system stability
- [ ] Implement deep sleep mode for battery operation

### Documentation
- [ ] Add Russian translations for ROADMAP.md and TODO.md
- [ ] Create video tutorial for hardware assembly
- [ ] Write troubleshooting guide for common issues
- [ ] Add API examples for each module
- [ ] Create Fritzing diagram for PCB design

---

## 🎯 Medium Priority (Next Month)

### Hardware Support
- [ ] Add support for ESP32-S3 (more RAM, USB OTG)
- [ ] Implement I2S microphone array (4x ICS-43434)
- [ ] Add support for external GPS antenna (active/passive)
- [ ] Design 3D-printable enclosure (STL files)
- [ ] Create PCB layout (KiCad files)

### Software Features
- [ ] Implement RSSI history graphing on OLED
- [ ] Add CSV export via web interface
- [ ] Implement OTA firmware updates
- [ ] Add WiFi station mode configuration via web UI
- [ ] Implement geofence import/export (KML format)

### ML/AI
- [ ] Train TFLite model on real drone RF signatures
- [ ] Implement model quantization (INT8) for faster inference
- [ ] Add confidence threshold configuration
- [ ] Implement online learning (model fine-tuning)
- [ ] Add support for multiple models (swap via web UI)

### Testing
- [ ] Write unit tests for protocol parsers
- [ ] Create integration tests for RF modules
- [ ] Add CI/CD pipeline (GitHub Actions)
- [ ] Implement hardware-in-the-loop testing
- [ ] Create test dataset for ML model validation

---

## 📋 Low Priority (Future)

### Advanced Features
- [ ] Implement 433 MHz support (second CC1101)
- [ ] Add 1.2 GHz video band detection
- [ ] Implement ESP-NOW mesh networking
- [ ] Add TDOA triangulation algorithm
- [ ] Implement servo-controlled directional antenna

### Integrations
- [ ] Add Home Assistant integration (MQTT)
- [ ] Implement Telegram bot for alerts
- [ ] Add Discord webhook notifications
- [ ] Integrate with FlightRadar24 API
- [ ] Add ADS-B receiver support (aircraft detection)

### UI/UX
- [ ] Redesign web dashboard (modern UI framework)
- [ ] Add dark/light theme toggle
- [ ] Implement multi-user authentication
- [ ] Add customizable alert sounds
- [ ] Create mobile app (React Native)

### Performance
- [ ] Optimize SPI bus speed (increase to 10 MHz)
- [ ] Implement DMA for SD card writes
- [ ] Add PSRAM support for larger buffers
- [ ] Optimize ML inference (use ESP-NN library)
- [ ] Implement multi-core task distribution

---

## 🐛 Known Issues

### Critical
- [ ] Web server crashes after 10+ WebSocket connections
- [ ] GPS module conflicts with LoRa on shared pins
- [ ] SD card write failures during high RF activity
- [ ] Memory fragmentation after 24h continuous operation

### Major
- [ ] Remote ID BLE scanning misses some packets
- [ ] ML classifier returns false positives on WiFi signals
- [ ] OLED display flickers during SPI bus contention
- [ ] Meshtastic packets occasionally corrupted

### Minor
- [ ] Serial output formatting inconsistent
- [ ] GPS fix takes >60s in urban environments
- [ ] Web dashboard doesn't update RSSI smoothly
- [ ] Log file rotation doesn't delete oldest files correctly

---

## 💡 Feature Requests (Community)

### From GitHub Issues
- [ ] Add support for RTL-SDR dongles (#12)
- [ ] Implement frequency scanner mode (#18)
- [ ] Add support for external amplifiers (#23)
- [ ] Create Windows/Linux desktop app (#27)
- [ ] Add support for multiple OLED displays (#31)

### From Reddit/Forums
- [ ] Add audio alerts (buzzer/speaker)
- [ ] Implement battery voltage monitoring
- [ ] Add support for external GPS (u-blox M8N)
- [ ] Create Raspberry Pi version
- [ ] Add support for directional antennas (Yagi, patch)

---

## 🔧 Technical Debt

### Code Quality
- [ ] Refactor main.cpp (split into modules)
- [ ] Add Doxygen comments to all functions
- [ ] Implement consistent error codes
- [ ] Add input validation for all user inputs
- [ ] Remove hardcoded constants (move to config)

### Architecture
- [ ] Implement proper state machine for system modes
- [ ] Add event-driven architecture (pub/sub pattern)
- [ ] Separate hardware abstraction layer (HAL)
- [ ] Implement dependency injection for testability
- [ ] Add logging framework (ESP_LOG macros)

### Security
- [ ] Add HTTPS support for web server
- [ ] Implement API authentication (JWT tokens)
- [ ] Add input sanitization for web forms
- [ ] Implement secure boot (ESP32 feature)
- [ ] Add encrypted storage for sensitive data

---

## 📊 Metrics & Analytics

### Performance Targets
- [ ] Reduce boot time to <5 seconds
- [ ] Achieve 10 Hz RSSI update rate per module
- [ ] Reduce ML inference time to <50ms
- [ ] Achieve 99.9% uptime (24h stress test)
- [ ] Reduce power consumption to <300mA average

### Quality Targets
- [ ] 95% code coverage (unit tests)
- [ ] Zero memory leaks (Valgrind/ASAN)
- [ ] <5% false positive rate (ML classifier)
- [ ] <1% packet loss (Meshtastic mesh)
- [ ] 100% documentation coverage (API reference)

---

## 🎓 Learning Resources

### For Contributors
- [ ] Create "Getting Started" guide for developers
- [ ] Write architecture overview document
- [ ] Create video tutorial series (YouTube)
- [ ] Add code walkthrough comments
- [ ] Create FAQ document

### For Users
- [ ] Write beginner's guide to RF detection
- [ ] Create antenna tuning tutorial
- [ ] Add legal compliance checklist
- [ ] Write field deployment guide
- [ ] Create troubleshooting flowchart

---

## 🚀 Release Checklist (v0.3.0)

- [ ] Complete all High Priority tasks
- [ ] Fix all Critical bugs
- [ ] Update all documentation
- [ ] Create release notes
- [ ] Tag release in Git
- [ ] Build and test firmware
- [ ] Update README.md with new features
- [ ] Announce release on GitHub/Reddit/Forums

---

**Last Updated**: March 28, 2026  
**Next Review**: April 15, 2026
