# SkyPort Rev C.1 electrical contract

SkyPort is a **keyed, power-limited internal expansion interface**. It becomes a product name only when the connector, protection parts and firmware contract in this document are present in the released schematic/PCB and pass first-article tests. It is not a general-purpose unprotected GPIO header.

## Connector and pinout

**Connector family:** JST GH, 1.25 mm pitch, 16-circuit keyed receptacle, `BM16B-GHS-TBT(LF)(SN)` on motherboard; mating harness `GHR-16V-S` with specified crimp contacts. This is mechanically polarized, through-hole-retained, available through Chinese harness/PCBA channels, and suitable for internally routed modules. Do not expose the unmated connector on an exterior user surface.

| Pin | Signal | Direction | Electrical requirement |
|---:|---|---|---|
| 1 | GND | — | First-mate ground; return for all power and signals. |
| 2 | GND | — | Parallel power/RF return; second-mate ground. |
| 3 | `SKYPORT_5V_SW` | output | 5.0 V nominal, current limited to **100 mA total**. Load switch, reverse-current blocking and resettable protection required. Not a battery charger input. |
| 4 | `3V3` | output | 3.3 V ±5%, **100 mA total** allocated; protected by a separately reviewed load switch. |
| 5 | `I2C_SDA` | bidirectional | Open-drain 3.3 V; motherboard pull-ups are the only default pull-ups. 100 kHz default; 400 kHz only after bus-capacitance validation. |
| 6 | `I2C_SCL` | output/open-drain | Open-drain 3.3 V; same bus requirements as SDA. |
| 7 | `SPI_SCK` | output | 3.3 V logic through 33 Ω source series resistor. |
| 8 | `SPI_MOSI` | output | 3.3 V logic through 33 Ω source series resistor. |
| 9 | `SPI_MISO` | input | 3.3 V only; module must tri-state unless selected. |
| 10 | `SPI_CS_N` | output | Assigned by an MCP23008 GPIO-expander bit. Active low. |
| 11 | `MODULE_IRQ_N` | input | Open-drain, pulled up to 3.3 V; terminates at the GPIO expander interrupt aggregation. |
| 12 | `MODULE_ENABLE` | output | 3.3 V GPIO-expander output. Low while power is off and throughout boot. |
| 13 | `MODULE_ID` | bidirectional | I²C EEPROM `0x57` identification address; no static MCU pin. |
| 14 | `UART_TX_3V3` | output | SC16IS750 I²C-UART bridge output, 3.3 V only. |
| 15 | `UART_RX_3V3` | input | SC16IS750 I²C-UART bridge input, 3.3 V only. |
| 16 | `RESERVED` | — | Grounded on motherboard through 0 Ω DNP option; do not use without a new interface revision. |

## Protection and sequencing

- All externally reachable SkyPort pins need a low-capacitance ESD array at the connector. I²C, GPIO and UART need series resistance selected against edge rate and bus capacitance. Do not place a high-capacitance protection device on SPI without measurement.
- `SKYPORT_5V_SW` and `3V3` must be off at reset. Firmware reads the module descriptor first, validates voltage/current class and only then asserts `MODULE_ENABLE`.
- The Rev C.1 current allocation is intentionally limited. An SDR or display module requiring more power uses a separately powered module with signal isolation; it must not borrow unlimited current from the 1S boost rail.
- A module must contain no RF transmitter, jamming function, injection function or charger input under the canonical passive-product scope.

## Discovery descriptor

The required `0x57` I²C EEPROM record is fixed-size and CRC protected:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | ASCII `SKYP` magic |
| 4 | 1 | interface major version (`1`) |
| 5 | 1 | interface minor version |
| 6 | 2 | module type |
| 8 | 2 | requested 5 V current, mA |
| 10 | 2 | requested 3.3 V current, mA |
| 12 | 4 | feature flags |
| 16 | 16 | ASCII module MPN, NUL terminated |
| 32 | 4 | CRC-32 of bytes 0–31 |

Firmware rejects an invalid CRC, an unknown major version, a power request above the allocation, or a module declaring an active RF feature. It logs the descriptor and power decision as raw evidence.

## Firmware API contract

`SkyPortModule` implementations receive only the granted bus handles and power state. They publish `BandObservation` events, never final brand identity. The detector fuses observations using timestamp, source band, relative level, duration, duty cycle and evidence flags. This permits future 400–470 MHz, 1.2 GHz, SDR, alternate-video and fixed-node modules without motherboard pin reassignment.
