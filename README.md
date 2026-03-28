# SkySweep32

Multi-band passive drone detector based on ESP32 microcontroller. Scans radio spectrum across three frequency bands (900 MHz, 2.4 GHz, 5.8 GHz) to detect UAV control signals and video transmission.

## Hardware Components

- **ESP32 DevKit** - Main controller
- **CC1101** - 900 MHz transceiver module
- **NRF24L01+** - 2.4 GHz transceiver module
- **RX5808** - 5.8 GHz video receiver module
- **OLED Display (I2C)** - 128x64 SSD1306

## Pinout Configuration

### SPI Bus (Shared)
| Signal | ESP32 Pin |
|--------|-----------|
| MOSI   | GPIO 23   |
| MISO   | GPIO 19   |
| SCK    | GPIO 18   |

### Chip Select Pins (Individual)
| Module      | CS Pin    |
|-------------|-----------|
| CC1101      | GPIO 5    |
| NRF24L01+   | GPIO 17   |
| RX5808      | GPIO 16   |

### I2C Bus (OLED Display)
| Signal | ESP32 Pin |
|--------|-----------|
| SDA    | GPIO 21   |
| SCL    | GPIO 22   |

### Additional Connections
- **RX5808 RSSI**: GPIO 34 (ADC1_CH6)
- **Power**: 3.3V and GND to all modules

## Build Instructions

```bash
pio run
pio run --target upload
pio device monitor
```

## Architecture

The system operates in round-robin mode, sequentially polling each RF module for RSSI levels. Detection thresholds and signal patterns are analyzed to identify potential drone activity.
