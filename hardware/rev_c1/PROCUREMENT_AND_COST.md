# Rev C.1 procurement and preliminary China cost matrix

**All prices below are budgetary engineering estimates, not supplier quotations.** They are intentionally stated as ranges because 2026 domestic availability, authorized provenance, PCBA feeder support and counterfeit risk cannot be established from a BOM alone. Request quotes for two assembled PCBAs plus five spare bare boards before release.

| Assembly / part group | Exact manufacturer / MPN | China / PCBA disposition | Substitution policy | Budgetary CNY / unit |
|---|---|---|---|---:|
| ESP32 module | Espressif `ESP32-S3-WROOM-1-N16R8` | Usually catalogue/PCBA available; confirm reel provenance | No alternate module without pin, PSRAM, antenna and firmware review | 35–65 |
| 2.4 GHz receiver | Ebyte `E28-2G4M12SX` | Domestic source; PCBA fit after lot check | No alternate E28/SX1281 variant | 25–50 |
| 855–925 MHz receiver | Ebyte `E07-900M10S` | Domestic source; PCBA fit after lot check | No alternate radio/module | 25–50 |
| GNSS | u-blox `SAM-M10Q-00B` | Authorized/consigned preferred; RF module placement capable PCBA | No substitute without antenna/mechanical review | 65–120 |
| Analog 5.8 receiver | Exact 12-pad `RX5808-2012-12P` | **Consigned only** until lot is qualified | No "RX5808-like" replacement; DNP if not qualified | 30–70 |
| 5.x detector | ADI `AD8318ACPZ-R7` | **Consigned/authorized only** | No substitution absent calibrated response review | 45–100 |
| 5.x switching | pSemi `PE42427` ×2 | **Consigned/authorized only** | No lower-frequency or different-topology switch | 60–150 |
| 5.x filters | Johanson `5515BP15B0725001E`, `6530BP44A1190001E` | **Consigned/authorized only** | Must match passband/S-parameters; recharacterize any candidate | 35–100 |
| Video / data interface | TI `THS7313PWR`, `LM1881M/NOPB`, `ADS1115IDGSR`, Microchip `MCP23008-I/SS` | PCBA capable after quote; LM1881 consignment preferred | Electrical-performance review required | 35–90 |
| Power, USB, gauge | Exact existing TI/ADI/Diodes/Nexperia MPNS | Standard PCBA components subject to stock | Match electrical/thermal/footprint; no unreviewed substitution | 70–130 |
| Connectors, passives, PCB | Exact BOM; 4-layer ENIG controlled impedance | Domestic PCBA capable after DFM/coupon confirmation | Fabricator may change impedance geometry only with field-solve approval | 100–180 |
| Battery, OLED, antennas, enclosure | Exact qualified external items | Usually separately procured/assembled | Antenna/battery/OLED changes require mechanical/RF review | 180–320 |
| PCBA setup, stencil, assembly | Quoted by selected assembler | Two-unit run carries high NRE | No estimate may replace a quote | 150–350 |

**Budgetary complete device:** **850–1,600 CNY per first-article unit** at two assembled boards, before shipping, tax, fixture/VNA time and rework. A five-to-ten unit run may reduce setup allocation; it does not remove RF-part procurement risk. This is above the original 500 CNY neighborhood because traceable wideband RF switching/filtering and qualified assembly are real cost drivers. A robust, measured first article is preferred to a falsely cheap substitute BOM.

## Required purchase gates

1. Quote the actual PCB stackup with differential and 50 Ω impedance coupons before ordering boards.
2. Obtain lot traceability/reel photos for AD8318, PE42427, both Johanson filters, SAM-M10Q and RX5808.
3. Have the PCBA house confirm 2×2 mm QFN, 0805/1812 RF filters, RX5808 12-pad footprint, GNSS module and all connector operations.
4. Consign each RF-risk part if the assembler cannot provide authorized traceability.
5. Buy RX5808 spare lots for incoming qualification before authorizing board placement.
6. Reject all alternate module substitutions, reclaimed ICs and untraceable RF parts.
