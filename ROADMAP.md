# SkySweep32 Development Roadmap

## Current Status (v0.2.0 - March 2026)

✅ **Implemented Features**:
- Multi-band RF scanning (900 MHz, 2.4 GHz, 5.8 GHz)
- Remote ID detection (FAA ANSI/CTA-2063)
- Web dashboard with real-time WebSocket
- SD card forensic logging
- GPS geolocation and geofencing
- Meshtastic LoRa mesh networking
- TensorFlow Lite Micro ML classification
- MAVLink/CRSF protocol parsing
- Active countermeasures (legal authorization required)

---

## Phase 1: Enhanced Detection (Q2 2026) 🎯 PRIORITY

### 1.1 Extended Frequency Coverage
**Goal**: Close blind spots in non-standard frequency bands

- [ ] **433 MHz Support** (Budget: $5)
  - Add second CC1101 module configured for 433 MHz
  - Detect long-range RC links (FrSky R9, TBS Crossfire LR)
  - Wire antenna: 17.3cm quarter-wave
  - Driver: Extend existing CC1101Driver with frequency profiles

- [ ] **1.2-1.3 GHz Video Band** (Budget: $15-20)
  - Add Partom 1.2G receiver module or similar
  - Detect analog video on 1.2-1.3 GHz (long-range FPV)
  - New driver: `rx1200.cpp/h` based on RX5808 architecture
  - SPI CS pin: GPIO 13

- [ ] **3.3 GHz / 7-10 GHz Detection** (Budget: $50-80)
  - Research: Spectrum analyzer modules (HackRF One clone, TinySA Ultra)
  - Integration: UART/USB interface to ESP32
  - Detect exotic military/commercial drone bands

**Estimated Completion**: May 2026  
**Difficulty**: Medium  
**Impact**: High (covers 80% of non-standard drones)

---

### 1.2 Advanced Signal Analysis (TinyML)
**Goal**: Replace threshold-based detection with AI pattern recognition

- [ ] **Spectral Waterfall Capture**
  - Implement FFT on CC1101/NRF24 I/Q data
  - Store 128-point spectrogram snapshots
  - Buffer: 16KB circular buffer in PSRAM

- [ ] **Edge Impulse Integration**
  - Train model on ELRS, Crossfire, DJI, analog FPV signatures
  - Export TFLite model (<200KB)
  - Replace simple RSSI thresholds with confidence scores
  - Target: 95% accuracy, <100ms inference

- [ ] **FHSS Pattern Recognition**
  - Detect frequency-hopping patterns (ELRS, Crossfire)
  - Analyze hop timing and bandwidth
  - Distinguish from WiFi/Bluetooth interference

**Estimated Completion**: June 2026  
**Difficulty**: High  
**Impact**: Critical (reduces false positives by 90%)

---

### 1.3 Enhanced Acoustic Detection
**Goal**: Improve rotor sound classification with ML

- [ ] **Audio Classification Model**
  - Replace Goertzel with TFLite Audio model
  - Train on 3", 5", 7" propeller sounds at various RPMs
  - Distinguish from wind, cars, helicopters
  - Dataset: Record 1000+ samples

- [ ] **Multi-Microphone Array** (Budget: $10-15)
  - Add 2-4 MEMS microphones (ICS-43434)
  - Implement beamforming for direction estimation
  - Calculate azimuth angle (±15° accuracy)

**Estimated Completion**: July 2026  
**Difficulty**: Medium  
**Impact**: Medium (complements RF detection)

---

## Phase 2: Distributed Detection Network (Q3 2026)

### 2.1 ESP-NOW Mesh Networking
**Goal**: Multi-node triangulation and collaborative detection

- [ ] **ESP-NOW Protocol Layer**
  - Replace/augment Meshtastic with ESP-NOW for low-latency
  - Broadcast detection events to all nodes
  - Synchronize timestamps (NTP or GPS PPS)

- [ ] **TDOA Triangulation**
  - Collect RSSI + timestamp from 3+ nodes
  - Calculate drone position using Time Difference of Arrival
  - Display on web dashboard map
  - Accuracy target: ±50m at 1km range

- [ ] **Swarm Coordination**
  - Automatic role assignment (master/slave nodes)
  - Load balancing (frequency scanning distribution)
  - Fault tolerance (node dropout handling)

**Estimated Completion**: August 2026  
**Difficulty**: High  
**Impact**: Critical (enables area coverage)

---

### 2.2 Directional Antenna Scanning
**Goal**: Azimuth detection with rotating antenna

- [ ] **Servo-Controlled Antenna Mount** (Budget: $20-30)
  - Add 2x SG90 servos (azimuth + elevation)
  - Mount directional antennas (Yagi, patch)
  - PWM control via ESP32 LEDC

- [ ] **360° Scanning Algorithm**
  - Rotate antenna in 10° increments
  - Record RSSI at each position
  - Identify peak signal direction
  - Display on OLED as compass bearing

- [ ] **Automatic Tracking Mode**
  - Lock onto strongest signal
  - Follow drone movement
  - Update web dashboard with live bearing

**Estimated Completion**: September 2026  
**Difficulty**: Medium  
**Impact**: High (precise threat localization)

---

## Phase 3: Advanced Countermeasures (Q4 2026) ⚠️ LEGAL AUTH REQUIRED

### 3.1 GPS Spoofing
**Goal**: Force commercial drones to land via fake GPS coordinates

- [ ] **HackRF One Integration** (Budget: $100-150)
  - UART/USB interface to ESP32
  - Generate fake GPS L1 C/A signals
  - Inject coordinates of no-fly zones (airports, prisons)
  - Target: DJI, Autel, Parrot drones

- [ ] **Automated Spoofing Logic**
  - Detect drone via Remote ID
  - Calculate fake coordinates (nearest NFZ)
  - Trigger HackRF transmission
  - Monitor drone RTH/landing

**Estimated Completion**: October 2026  
**Difficulty**: Very High  
**Impact**: Critical (non-destructive neutralization)  
**Legal Status**: ILLEGAL without authorization

---

### 3.2 Intelligent Jamming
**Goal**: Adaptive jamming based on protocol detection

- [ ] **Protocol-Specific Jamming**
  - ELRS: Target specific hop channels
  - Crossfire: Jam control packets only
  - DJI OcuSync: Disrupt video downlink
  - Analog: Wideband noise on detected channel

- [ ] **Power Management**
  - Dynamic TX power adjustment
  - Minimize collateral interference
  - Battery-efficient pulsed jamming

**Estimated Completion**: November 2026  
**Difficulty**: High  
**Impact**: High (energy-efficient countermeasures)

---

## Phase 4: Professional-Grade Hardware (2027)

### 4.1 SDR Integration
**Goal**: True wideband spectrum analysis

- [ ] **RTL-SDR V4 + Orange Pi Zero 2W** (Budget: $50-70)
  - Scan 24-1766 MHz in real-time
  - Detect any RF emission instantly
  - ESP32 as display/control interface
  - Python backend on Orange Pi

- [ ] **HackRF One (TX/RX)** (Budget: $300-400)
  - Full-duplex 1 MHz - 6 GHz
  - Replace all discrete RF modules
  - Software-defined jamming/spoofing
  - Professional-grade solution

**Estimated Completion**: Q1 2027  
**Difficulty**: Very High  
**Impact**: Transforms into commercial-grade system

---

### 4.2 Optical Detection
**Goal**: Detect radio-silent drones

- [ ] **ESP32-CAM Integration** (Budget: $10-15)
  - Add OV2640 camera module
  - TFLite object detection model
  - Detect drone silhouettes
  - Trigger RF scan on visual detection

- [ ] **Thermal Camera** (Budget: $100-200)
  - MLX90640 32x24 thermal array
  - Detect heat signatures at night
  - Complement RF/optical detection

**Estimated Completion**: Q2 2027  
**Difficulty**: High  
**Impact**: Medium (niche use case)

---

## Phase 5: Software & UX Improvements (Ongoing)

### 5.1 Web Dashboard Enhancements
- [ ] Real-time spectrum waterfall display
- [ ] Interactive map with drone positions
- [ ] Historical playback of detections
- [ ] Mobile-responsive design
- [ ] Multi-language support (EN/RU/CN)

### 5.2 Mobile App
- [ ] React Native app for iOS/Android
- [ ] Bluetooth LE connection to ESP32
- [ ] Push notifications for threats
- [ ] Offline map caching

### 5.3 Cloud Integration
- [ ] MQTT broker for remote monitoring
- [ ] Cloud-based ML model updates
- [ ] Threat intelligence sharing
- [ ] Fleet management dashboard

---

## Budget Summary

| Phase | Components | Estimated Cost |
|-------|-----------|----------------|
| Phase 1 | 433MHz CC1101, 1.2GHz RX, microphones | $30-50 |
| Phase 2 | Servos, antennas | $20-30 |
| Phase 3 | HackRF One | $100-150 |
| Phase 4 | RTL-SDR, Orange Pi, cameras | $160-285 |
| **Total** | | **$310-515** |

---

## Community Contributions Welcome

**Priority Areas**:
1. TinyML model training (ELRS/Crossfire datasets)
2. ESP-NOW mesh protocol implementation
3. Web dashboard UI/UX improvements
4. Multi-language documentation
5. Field testing and validation

**How to Contribute**: See [CONTRIBUTING.md](CONTRIBUTING.md)

---

## Legal Disclaimer

⚠️ **Phases 3+ involve active RF countermeasures that are ILLEGAL in most jurisdictions without explicit authorization. This roadmap is for educational and authorized defense purposes only.**

---

**Last Updated**: March 28, 2026  
**Maintainer**: SkySweep32 Development Team
