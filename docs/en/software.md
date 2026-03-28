# Software API Reference

## Architecture Overview

SkySweep32 uses a modular architecture with hardware abstraction layers (HAL) for RF modules and protocol parsers for signal analysis.

```
Application Layer (main.cpp)
    ↓
Countermeasure System (countermeasures.cpp)
    ↓
Protocol Parsers (mavlink_parser.cpp, crsf_parser.cpp)
    ↓
Hardware Drivers (cc1101.cpp, nrf24l01.cpp, rx5808.cpp)
    ↓
SPI/I2C Hardware Abstraction
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

### Compile-Time Flags

```cpp
// Enable active countermeasures (platformio.ini)
#define ENABLE_COUNTERMEASURES

// RSSI thresholds (countermeasures.cpp)
#define RSSI_THRESHOLD_LOW      45
#define RSSI_THRESHOLD_MEDIUM   60
#define RSSI_THRESHOLD_HIGH     75
#define RSSI_THRESHOLD_CRITICAL 85
```

### Pin Definitions

```cpp
// SPI pins (main.cpp)
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK  18

// Chip Select pins
#define CS_CC1101    5
#define CS_NRF24L01  17
#define CS_RX5808    16

// I2C pins
#define I2C_SDA 21
#define I2C_SCL 22

// Additional pins
#define RX5808_RSSI_PIN 34
#define NRF24_CE_PIN    4
```

## Serial Commands

The system outputs diagnostic information via Serial (115200 baud):

```
[CC1101] Chip version: 0x14
[NRF24] Initialized successfully
[RX5808] Set to Raceband CH1 (5658 MHz)
[CM] Countermeasure system initialized
[SYSTEM] Starting round-robin RSSI polling...

[CC1101] RSSI: -65 dBm | Threat: MEDIUM
[NRF24L01+] RSSI: -45 dBm | Threat: HIGH *** SIGNAL DETECTED ***
[RX5808] RSSI: 78% | Threat: HIGH
---
```

## Performance Metrics

- **Scan Rate**: ~3 Hz (all three bands)
- **RSSI Update Rate**: ~10 Hz per module
- **Protocol Detection Latency**: <100ms
- **Countermeasure Response Time**: <500ms

## Memory Usage

- **Flash**: ~450 KB (with countermeasures enabled)
- **RAM**: ~45 KB
- **Stack**: ~8 KB per task

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
