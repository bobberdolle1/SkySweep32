# Hardware Setup Guide

## Bill of Materials (BOM)

| Component | Quantity | Specifications | Approx. Cost |
|-----------|----------|----------------|--------------|
| ESP32 DevKit | 1 | 240MHz dual-core, WiFi/BT | $5-10 |
| CC1101 Module | 1 | 300-928MHz transceiver | $3-5 |
| NRF24L01+ Module | 1 | 2.4GHz transceiver with PA+LNA | $2-4 |
| RX5808 Module | 1 | 5.8GHz video receiver | $3-5 |
| OLED Display | 1 | 128x64 I2C SSD1306 | $3-5 |
| Breadboard/PCB | 1 | For prototyping | $2-5 |
| Jumper Wires | 20+ | Male-to-female | $2 |
| Power Supply | 1 | 5V 2A USB or battery | $5 |

**Total Cost**: ~$25-40 USD

## Wiring Diagram

See [wiring_diagram.svg](../wiring_diagram.svg) for visual reference.

## Step-by-Step Assembly

### 1. ESP32 Preparation

- Connect ESP32 to breadboard
- Ensure 3.3V and GND rails are accessible
- **IMPORTANT**: ESP32 GPIO pins are 3.3V tolerant only!

### 2. SPI Bus Wiring (Shared)

Connect the following pins from ESP32 to ALL three RF modules:

| ESP32 Pin | Signal | Connect To |
|-----------|--------|------------|
| GPIO 23 | MOSI | CC1101 MOSI, NRF24 MOSI, RX5808 MOSI |
| GPIO 19 | MISO | CC1101 MISO, NRF24 MISO, RX5808 MISO |
| GPIO 18 | SCK | CC1101 SCK, NRF24 SCK, RX5808 SCK |
| 3.3V | VCC | All modules VCC |
| GND | GND | All modules GND |

### 3. Chip Select (CS) Pins

Each SPI device needs a unique CS pin:

| Module | ESP32 CS Pin | Module CS Pin |
|--------|--------------|---------------|
| CC1101 | GPIO 5 | CSN/SS |
| NRF24L01+ | GPIO 17 | CSN |
| RX5808 | GPIO 16 | CS |

### 4. NRF24L01+ CE Pin

| ESP32 Pin | NRF24 Pin |
|-----------|-----------|
| GPIO 4 | CE |

### 5. RX5808 RSSI Pin

| ESP32 Pin | RX5808 Pin |
|-----------|------------|
| GPIO 34 (ADC) | RSSI |

### 6. OLED Display (I2C)

| ESP32 Pin | OLED Pin |
|-----------|----------|
| GPIO 21 | SDA |
| GPIO 22 | SCL |
| 3.3V | VCC |
| GND | GND |

## Module-Specific Notes

### CC1101

- **Antenna**: Requires 868/915MHz antenna (wire length ~8.2cm for 915MHz)
- **Power**: Draws ~15mA in RX mode, ~30mA in TX mode
- **Voltage**: 1.8-3.6V (3.3V recommended)

### NRF24L01+

- **Antenna**: Built-in PCB antenna or external SMA
- **Power**: PA+LNA version draws up to 115mA in TX mode
- **Voltage**: 1.9-3.6V (3.3V recommended)
- **Note**: Use 10µF capacitor between VCC and GND if experiencing instability

### RX5808

- **Antenna**: Requires 5.8GHz antenna (cloverleaf or patch)
- **Power**: ~100mA typical
- **Voltage**: 5V input (has onboard 3.3V regulator)
- **Note**: Connect VCC to ESP32 5V pin (VIN), not 3.3V

### OLED Display

- **I2C Address**: Usually 0x3C or 0x3D
- **Power**: ~20mA
- **Voltage**: 3.3V or 5V compatible

## Power Considerations

**Total Current Draw**:
- ESP32: ~160mA (WiFi active)
- CC1101: ~30mA
- NRF24L01+: ~115mA (TX mode)
- RX5808: ~100mA
- OLED: ~20mA
- **Total**: ~425mA peak

**Recommended Power Supply**: 5V 2A USB adapter or LiPo battery with voltage regulator.

## Antenna Recommendations

### 900 MHz (CC1101)
- **Type**: Quarter-wave wire antenna
- **Length**: 8.2cm for 915MHz
- **Gain**: 0-2dBi

### 2.4 GHz (NRF24L01+)
- **Type**: PCB antenna (built-in) or external dipole
- **Gain**: 2-5dBi (PA+LNA version)

### 5.8 GHz (RX5808)
- **Type**: Cloverleaf (RHCP/LHCP) or patch antenna
- **Gain**: 2-8dBi
- **Connector**: SMA or RP-SMA

## Enclosure Design

Recommended enclosure specifications:
- **Material**: ABS plastic or 3D-printed PLA
- **Dimensions**: 120mm x 80mm x 40mm minimum
- **Features**:
  - Antenna ports (SMA bulkhead connectors)
  - Ventilation holes for heat dissipation
  - OLED display cutout
  - USB access for programming/power

## Testing Procedure

1. **Visual Inspection**: Check all connections for shorts
2. **Power Test**: Connect power, verify 3.3V on all module VCC pins
3. **Serial Monitor**: Upload firmware, check initialization messages
4. **Module Detection**: Verify each RF module responds (check serial output)
5. **RSSI Test**: Place active 2.4GHz device nearby, verify detection on NRF24
6. **Display Test**: Confirm OLED shows signal bars

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|----------------|----------|
| ESP32 won't boot | GPIO0 pulled low | Check wiring, ensure GPIO0 is floating |
| Module not detected | SPI wiring error | Verify MOSI/MISO not swapped |
| NRF24 unstable | Power supply noise | Add 10µF capacitor near VCC pin |
| RX5808 no signal | Wrong voltage | Ensure RX5808 VCC connected to 5V |
| OLED blank | Wrong I2C address | Try 0x3C or 0x3D in code |

## Safety Warnings

⚠️ **RF Exposure**: Keep antennas at least 20cm from body during operation.

⚠️ **ESD Protection**: Handle modules with anti-static precautions.

⚠️ **Overheating**: Ensure adequate ventilation, especially for PA+LNA modules.

## Next Steps

After hardware assembly, proceed to [Software Configuration](software.md).
