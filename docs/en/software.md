# Software API Reference

## Software Architecture

SkySweep32 uses a modular architecture with hardware abstraction layers (HAL) for RF modules, protocol parsers for signal analysis, and 6 new feature modules for enhanced detection capabilities.

```
Application Layer (main.cpp)
    ↓
┌─────────────────────────────────────────────────────────────┐
│  Feature Modules (New)                                      │
│  ├── Remote ID Detector (WiFi/BLE scanning)                │
│  ├── Web Server (WebSocket dashboard)                      │
│  ├── Data Logger (SD card forensics)                       │
│  ├── GPS Module (geolocation/geofencing)                   │
│  ├── Meshtastic Client (LoRa mesh)                         │
│  └── ML Classifier (TensorFlow Lite Micro)                 │
└─────────────────────────────────────────────────────────────┘
    ↓
Countermeasure System (countermeasures.cpp)
    ↓
Protocol Parsers (mavlink_parser.cpp, crsf_parser.cpp)
    ↓
Hardware Drivers (cc1101.cpp, nrf24l01.cpp, rx5808.cpp)
    ↓
SPI/I2C/UART Hardware Abstraction
```

## New Feature APIs

### RemoteIDDetector (FAA ANSI/CTA-2063)

```cpp
RemoteIDDetector remoteID(5000); // 5s scan interval

// Initialization
bool begin();

// Scanning
void update();                          // Call in loop()
uint8_t getDetectedDroneCount() const;
DroneRemoteIDData getDroneData(uint8_t index) const;

// Geofencing
bool isDroneInRange(double lat, double lon, float radiusMeters);

// Utility
void clearDetections();
void printDroneInfo(uint8_t index);
```

**DroneRemoteIDData Structure**:
```cpp
struct DroneRemoteIDData {
    char uasID[21];              // UAS Serial Number
    uint8_t idType;              // 0=None, 1=Serial, 2=CAA, 3=UTM
    uint8_t uaType;              // 0=None, 1=Aeroplane, 2=Helicopter, 3=Multirotor
    double latitude;             // Drone position
    double longitude;
    float altitude;              // Meters WGS84
    float speed;                 // m/s
    double operatorLatitude;     // Operator position
    double operatorLongitude;
    int8_t rssi;
    uint32_t lastSeen;
    bool isValid;
};
```

**Example**:
```cpp
remoteID.begin();
// In loop():
remoteID.update();
for (uint8_t i = 0; i < remoteID.getDetectedDroneCount(); i++) {
    DroneRemoteIDData drone = remoteID.getDroneData(i);
    Serial.printf("Drone: %s at %.6f,%.6f\n", 
                 drone.uasID, drone.latitude, drone.longitude);
}
```

### SkySweepWebServer (Real-time Dashboard)

```cpp
SkySweepWebServer webServer;

// Initialization
bool begin(bool accessPointMode = true);  // true = AP mode, false = STA mode
void stop();
void update();                            // Call in loop()

// Broadcasting
void broadcastRFData(const char* moduleName, int rssi, bool active);
void broadcastDroneDetection(const char* droneID, double lat, double lon, float alt);
void broadcastSystemStatus(const char* status);

// Configuration
void setConfig(const WebServerConfig& newConfig);
IPAddress getIPAddress() const;
uint16_t getConnectedClients() const;
```

**Access Dashboard**: `http://192.168.4.1` (default AP mode)

**Example**:
```cpp
webServer.begin(true);  // Start as Access Point
// In loop():
webServer.update();
webServer.broadcastRFData("CC1101", rssi, true);
```

### DataLogger (SD Card Forensics)

```cpp
DataLogger dataLogger;

// Initialization
bool begin(uint8_t csPin = 15);
void end();

// Logging
bool logRFData(const char* module, int rssi, float freq, const char* protocol);
bool logDroneRemoteID(const char* uasID, double lat, double lon, float alt, int rssi);
bool logSystemEvent(LogLevel level, const char* message);

// Export
bool exportToJSON(const char* outputFile, uint32_t startTime, uint32_t endTime);
bool exportToCSV(const char* outputFile, uint32_t startTime, uint32_t endTime);

// Configuration
void setLogLevel(LogLevel level);       // LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR
void setRotationSize(uint32_t sizeBytes);

// Status
uint32_t getTotalLogSize();
bool isSDCardAvailable() const;
```

**Example**:
```cpp
dataLogger.begin(15);  // CS pin 15
dataLogger.setLogLevel(LOG_INFO);
dataLogger.logRFData("NRF24", -65, 2400.0, "Unknown");
dataLogger.logDroneRemoteID("DJI-ABC123", 37.7749, -122.4194, 120.5, -70);
```

### GPSModule (Geolocation & Geofencing)

```cpp
GPSModule gpsModule;

// Initialization
bool begin(uint32_t baudRate = 9600);
void update();                          // Call in loop()

// Data access
GPSData getData() const;
bool isFixValid() const;

// Geofencing
void addGeofence(const char* name, double lat, double lon, float radius, bool alertOnEntry = true);
void removeGeofence(const char* name);
bool checkGeofenceViolations(double lat, double lon);

// Navigation
float getDistanceToPoint(double targetLat, double targetLon);
uint16_t getBearingToPoint(double targetLat, double targetLon);

// Utility
void printInfo();
```

**GPSData Structure**:
```cpp
struct GPSData {
    double latitude;
    double longitude;
    float altitude;
    float speed;
    uint16_t course;
    uint8_t satellites;
    float hdop;
    bool isValid;
    uint32_t lastUpdate;
};
```

**Example**:
```cpp
gpsModule.begin(9600);
gpsModule.addGeofence("Protected Zone", 37.7749, -122.4194, 1000.0, true);
// In loop():
gpsModule.update();
if (gpsModule.isFixValid()) {
    GPSData gps = gpsModule.getData();
    Serial.printf("Position: %.6f,%.6f\n", gps.latitude, gps.longitude);
}
```

### MeshtasticClient (LoRa Mesh Networking)

```cpp
MeshtasticClient meshtastic;

// Initialization
bool begin(float frequency = 915.0);    // 915MHz US, 868MHz EU
void update();                          // Call in loop()

// Broadcasting
bool broadcastDetectionAlert(const DetectionAlert& alert);
bool sendDirectMessage(uint32_t targetNodeID, const char* message);

// Packet management
uint8_t getReceivedPacketCount() const;
MeshPacket getPacket(uint8_t index) const;
void clearPackets();

// Network info
uint8_t getKnownNodeCount() const;
uint32_t getNodeID(uint8_t index) const;

// Configuration
void setTransmitInterval(uint32_t intervalMs);
int16_t getLastRSSI() const;
float getLastSNR() const;
```

**DetectionAlert Structure**:
```cpp
struct DetectionAlert {
    double latitude;
    double longitude;
    float altitude;
    char droneID[32];
    int rssi;
    uint8_t threatLevel;
    uint32_t timestamp;
};
```

**Example**:
```cpp
meshtastic.begin(915.0);
meshtastic.setTransmitInterval(30000);  // 30s between transmits
// In loop():
meshtastic.update();
if (threatLevel >= THREAT_CRITICAL) {
    DetectionAlert alert = {gps.latitude, gps.longitude, gps.altitude, 
                           "THREAT_DETECTED", rssi, threatLevel, millis()};
    meshtastic.broadcastDetectionAlert(alert);
}
```

### MLClassifier (TensorFlow Lite Micro)

```cpp
MLClassifier mlClassifier;

// Initialization
bool begin();
bool loadModel(const uint8_t* modelData, size_t modelSize);

// Classification
ClassificationResult classify(const RFFeatures& features);
ClassificationResult classifyFromRSSI(const int* rssiValues, size_t count);

// Utility
const char* getClassName(DroneClassification droneClass);
bool isModelLoaded() const;
void printClassificationResult(const ClassificationResult& result);
```

**DroneClassification Enum**:
```cpp
enum DroneClassification {
    CLASS_UNKNOWN = 0,
    CLASS_DJI_PHANTOM = 1,
    CLASS_DJI_MAVIC = 2,
    CLASS_FPV_RACING = 3,
    CLASS_MILITARY = 4
};
```

**Example**:
```cpp
mlClassifier.begin();
// Collect RSSI history
int rssiHistory[32];
// ... populate with readings
ClassificationResult result = mlClassifier.classifyFromRSSI(rssiHistory, 32);
if (result.isValid) {
    Serial.printf("Classified as: %s (%.1f%% confidence)\n",
                 mlClassifier.getClassName(result.droneClass),
                 result.confidence * 100.0f);
}
```

## Driver API

### CC1101Driver (900 MHz)

```cpp
CC1101Driver cc1101(CS_PIN);

// Initialization
bool begin();                           // Initialize module
void reset();                           // Hardware reset
void setFrequency(uint32_t frequencyHz); // Set center frequency
void setModulation(uint8_t modulation); // Set modulation type
void setDataRate(uint32_t baud);        // Set data rate

// Operation modes
void setRxMode();                       // Enter receive mode
void setTxMode();                       // Enter transmit mode
void setIdleMode();                     // Enter idle mode

// Signal analysis
int8_t readRSSI();                      // Read RSSI in dBm
uint8_t getLQI();                       // Link Quality Indicator
bool isCarrierDetected();               // Carrier sense

// Data transfer
void transmitData(uint8_t* data, uint8_t length);
uint8_t receiveData(uint8_t* buffer, uint8_t maxLength);
```

**Example Usage**:
```cpp
cc1101.begin();
cc1101.setFrequency(915000000); // 915 MHz
cc1101.setRxMode();
int8_t rssi = cc1101.readRSSI();
Serial.printf("RSSI: %d dBm\n", rssi);
```

### NRF24L01Driver (2.4 GHz)

```cpp
NRF24L01Driver nrf24(CS_PIN, CE_PIN);

// Initialization
bool begin();
void powerUp();
void powerDown();

// Configuration
void setChannel(uint8_t channel);       // 0-125 (2400-2525 MHz)
void setDataRate(uint8_t rate);         // 0=250kbps, 1=1Mbps, 2=2Mbps
void setPowerLevel(uint8_t level);      // 0-3 (min to max)
void setPayloadSize(uint8_t size);      // 1-32 bytes

// Operation
void startListening();                  // Enter RX mode
void stopListening();                   // Exit RX mode
int8_t readRSSI();                      // Estimated RSSI
uint8_t getRPD();                       // Received Power Detector

// Data transfer
void transmitData(uint8_t* data, uint8_t length);
uint8_t receiveData(uint8_t* buffer, uint8_t maxLength);
```

**Example Usage**:
```cpp
nrf24.begin();
nrf24.setChannel(76);  // 2.476 GHz
nrf24.setDataRate(2);  // 2 Mbps
nrf24.startListening();
uint8_t rpd = nrf24.getRPD();
```

### RX5808Driver (5.8 GHz)

```cpp
RX5808Driver rx5808(CS_PIN, RSSI_PIN);

// Initialization
bool begin();

// Channel selection
void setChannel(uint8_t band, uint8_t channel);
void setFrequency(uint16_t frequencyMHz);

// Signal measurement
int readRSSI();                         // 0-100 percentage
int readRSSIRaw();                      // Raw ADC value

// Scanning
void scanBand(uint8_t band, int* rssiValues);
RX5808Channel findStrongestChannel();

// Utility
uint16_t getCurrentFrequency();
const char* getBandName(uint8_t band);
```

**Frequency Bands**:
- `RX5808_BAND_A`: Boscam A (5725-5865 MHz)
- `RX5808_BAND_B`: Boscam B (5733-5866 MHz)
- `RX5808_BAND_E`: Boscam E/DJI (5645-5945 MHz)
- `RX5808_BAND_F`: Fatshark (5740-5880 MHz)
- `RX5808_BAND_R`: Raceband (5658-5917 MHz)

**Example Usage**:
```cpp
rx5808.begin();
rx5808.setChannel(RX5808_BAND_R, 0); // Raceband CH1
int rssi = rx5808.readRSSI();
Serial.printf("5.8GHz RSSI: %d%%\n", rssi);
```

## Protocol Parser API

### MAVLinkParser

```cpp
MAVLinkParser mavlink;

// Parsing
bool parseByte(uint8_t byte);
bool parseBuffer(uint8_t* data, uint16_t length);
MAVLinkPacket getPacket();

// Packet builders (for injection)
void buildRTHCommand(uint8_t* buffer, uint8_t* length);
void buildLandCommand(uint8_t* buffer, uint8_t* length);
void buildDisarmCommand(uint8_t* buffer, uint8_t* length);

// Analysis
bool isHeartbeat(MAVLinkPacket* packet);
bool isGPSData(MAVLinkPacket* packet);
MAVLinkHeartbeat parseHeartbeat(MAVLinkPacket* packet);
MAVLinkGPS parseGPS(MAVLinkPacket* packet);
```

**Example Usage**:
```cpp
uint8_t rxData[64];
uint8_t len = cc1101.receiveData(rxData, 64);
if (mavlink.parseBuffer(rxData, len)) {
    MAVLinkPacket pkt = mavlink.getPacket();
    if (mavlink.isHeartbeat(&pkt)) {
        MAVLinkHeartbeat hb = mavlink.parseHeartbeat(&pkt);
        Serial.printf("Vehicle type: %s\n", 
                     mavlink.getVehicleType(hb.type));
    }
}
```

### CRSFParser

```cpp
CRSFParser crsf;

// Parsing
bool parseByte(uint8_t byte);
bool parseBuffer(uint8_t* data, uint16_t length);
CRSFPacket getPacket();

// Packet builders
void buildRCChannels(uint8_t* buffer, uint8_t* length, uint16_t* channels);

// Analysis
bool isLinkStats(CRSFPacket* packet);
bool isGPS(CRSFPacket* packet);
CRSFLinkStats parseLinkStats(CRSFPacket* packet);
int8_t getRSSIFromLinkStats(CRSFLinkStats* stats);
```

**Example Usage**:
```cpp
uint8_t rxData[64];
uint8_t len = cc1101.receiveData(rxData, 64);
if (crsf.parseBuffer(rxData, len)) {
    CRSFPacket pkt = crsf.getPacket();
    if (crsf.isLinkStats(&pkt)) {
        CRSFLinkStats stats = crsf.parseLinkStats(&pkt);
        int8_t rssi = crsf.getRSSIFromLinkStats(&stats);
        Serial.printf("CRSF RSSI: %d dBm\n", rssi);
    }
}
```

## Countermeasure System API

```cpp
CountermeasureSystem cm;

// System control
void initialize();
void armSystem(bool armed);
bool isArmed();

// Threat assessment
ThreatLevel assessThreat(uint8_t moduleIndex, int rssiValue);
ThreatData getCurrentThreat();

// Manual countermeasures
bool executeCountermeasure(CountermeasureType type, uint8_t chipSelectPin);

// Automatic response
void autoRespond(uint8_t moduleIndex, int rssiValue, uint8_t chipSelectPin);
```

**Threat Levels**:
- `THREAT_NONE`: No threat detected
- `THREAT_LOW`: RSSI 45-59 dBm
- `THREAT_MEDIUM`: RSSI 60-74 dBm
- `THREAT_HIGH`: RSSI 75-84 dBm
- `THREAT_CRITICAL`: RSSI ≥85 dBm

**Countermeasure Types**:
- `CM_JAMMING_BROADBAND`: Wideband noise injection
- `CM_JAMMING_TARGETED`: Protocol-specific jamming
- `CM_PROTOCOL_INJECTION`: Command injection (RTH/Land)
- `CM_DEAUTH_FLOOD`: WiFi deauthentication

**Example Usage**:
```cpp
cm.initialize();
cm.armSystem(true);

ThreatLevel threat = cm.assessThreat(0, rssiValue);
if (threat >= THREAT_HIGH) {
    cm.executeCountermeasure(CM_PROTOCOL_INJECTION, CS_CC1101);
}
```

## Configuration Macros

### Feature Flags (main.cpp)

```cpp
// Enable/disable features at compile time
#define ENABLE_REMOTE_ID              // WiFi/BLE Remote ID scanning
#define ENABLE_WEB_SERVER             // HTTP/WebSocket dashboard
#define ENABLE_DATA_LOGGING           // SD card logging
#define ENABLE_GPS                    // GPS geolocation
#define ENABLE_MESHTASTIC             // LoRa mesh networking
#define ENABLE_ML_CLASSIFICATION      // TensorFlow Lite inference
#define ENABLE_ACOUSTIC_DETECTION     // Acoustic rotor detection
#define ENABLE_COUNTERMEASURES        // Active RF countermeasures (LEGAL AUTH REQUIRED)
```

### Compile-Time Flags (platformio.ini)

```ini
; Enable active countermeasures (REQUIRES LEGAL AUTHORIZATION)
build_flags = -DENABLE_COUNTERMEASURES

; Partition scheme for larger apps
board_build.partitions = huge_app.csv
```

### RSSI Thresholds (countermeasures.cpp)

```cpp
#define RSSI_THRESHOLD_LOW      45
#define RSSI_THRESHOLD_MEDIUM   60
#define RSSI_THRESHOLD_HIGH     75
#define RSSI_THRESHOLD_CRITICAL 85
```

### Pin Definitions (main.cpp)

```cpp
// SPI pins
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK  18

// Chip Select pins
#define CS_CC1101    5
#define CS_NRF24L01  17
#define CS_RX5808    16
#define CS_SD_CARD   15
#define CS_LORA      5   // Shared with CC1101 (mutually exclusive)

// I2C pins
#define I2C_SDA 21
#define I2C_SCL 22

// UART pins (GPS)
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

// LoRa pins
#define LORA_DIO0_PIN 2
#define LORA_DIO1_PIN 4
#define LORA_RESET_PIN 14

// Additional pins
#define RX5808_RSSI_PIN 34
#define NRF24_CE_PIN    4
```

## Serial Commands

The system outputs diagnostic information via Serial (115200 baud):

```
[SPI] Initialized - MOSI:23, MISO:19, SCK:18
[I2C] Initialized - SDA:21, SCL:22
[CC1101] Chip version: 0x14
[NRF24] Initialized successfully
[RX5808] Set to Raceband CH1 (5658 MHz)
[RemoteID] BLE and WiFi scanners ready
[WebServer] AP IP: 192.168.4.1
[WebServer] HTTP server started
[DataLogger] SD card size: 8192 MB
[DataLogger] Logging to: /logs/log_12345.txt
[GPS] GPS module ready
[Meshtastic] LoRa initialized (Node ID: 0xABCD1234)
[ML] ML classifier ready (model not loaded)
[CM] Countermeasure system initialized
[SYSTEM] Initialization complete
[SYSTEM] Starting round-robin RSSI polling...

[CC1101] RSSI: -65 dBm | Threat: MEDIUM
[NRF24L01+] RSSI: -45 dBm | Threat: HIGH *** SIGNAL DETECTED ***
[RX5808] RSSI: 78% | Threat: HIGH
[RemoteID] New drone detected: DJI-ABC123 (RSSI: -70 dBm)
[GPS] Position: 37.774900,-122.419400 @ 15.2m
[ML] Classified as: DJI Mavic (87.3% confidence)
[Meshtastic] Packet from 0x12345678 (RSSI: -95 dBm, SNR: 8.5 dB)
---
```

## Performance Metrics

### System Performance
- **Scan Rate**: ~3 Hz (all three RF bands)
- **RSSI Update Rate**: ~10 Hz per module
- **Protocol Detection Latency**: <100ms
- **Countermeasure Response Time**: <500ms

### New Feature Performance
- **Remote ID Scan Rate**: 5s interval (configurable)
- **Web Dashboard Update Rate**: Real-time via WebSocket
- **GPS Update Rate**: 1 Hz (configurable up to 10Hz)
- **Data Logging Write Speed**: ~50KB/s to SD card
- **LoRa Mesh Range**: Up to 10km line-of-sight
- **ML Inference Time**: 50-150ms per classification

## Memory Usage

### Flash Memory
- **Base System**: ~280 KB
- **With Countermeasures**: ~450 KB
- **With All 6 Features**: ~680 KB
- **TFLite Model**: ~100-200 KB (depends on model complexity)

### RAM Usage
- **Base System**: ~35 KB
- **With All Features**: ~65 KB
- **Stack per Task**: ~8 KB
- **WebSocket Buffers**: ~16 KB
- **SD Card Buffers**: ~4 KB

### Storage Requirements
- **Log Files**: ~1 MB per hour (typical detection activity)
- **ML Model**: 100-500 KB (stored in SPIFFS or SD card)

## Error Handling

All driver functions return `bool` for success/failure:

```cpp
if (!cc1101.begin()) {
    Serial.println("[ERROR] CC1101 initialization failed");
    // Fallback or retry logic
}
```

## Debugging Tips

1. **Enable verbose logging**: Add `#define DEBUG_VERBOSE` in main.cpp
2. **Check SPI communication**: Use logic analyzer on MOSI/MISO/SCK
3. **Verify module power**: Measure VCC pins with multimeter
4. **Test modules individually**: Comment out other modules in setup()

## Next Steps

- Review [Legal Compliance](legal.md) before enabling countermeasures
- See [Hardware Guide](hardware.md) for wiring verification
