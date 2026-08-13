# Rev C.1 passive detector architecture

## Decision

Rev C.1 uses a **two-bin switched 5.x RF power-observation receiver**, plus the retained RX5808 analog-video receiver. It is not an SDR, demodulator, spectrum analyzer, or protocol classifier.

```
J8 5.x 50 Ω SMA
       |
  PE42427 input SPDT
       |---- 5515BP15B0725001E (5.150–5.875 GHz) ----|
       |                                               |---- PE42427 output SPDT
       |---- 6530BP44A1190001E (5.925–6.000 GHz used) -|             |
                                                                  AD8318
                                                                    |
                                                               ADS1115 I²C ADC
                                                                    |
                                                                 ESP32-S3

J7 5.8 GHz SMA -> RX5808-2012-12P -> RSSI and VIDEO
                                             |             |
                                    ADS1115 input      THS7313 buffer
                                                       |           |
                                                75 Ω VIDEO OUT   LM1881 sync separator
                                                                         |
                                                            3.3 V level translation
                                                                         |
                                                             GPIO41 VIDEO_SYNC
```

The paired `PE42427` switches select the same filter path at input and output. A single switch followed by two filters tied at the detector would leave an unterminated parasitic filter branch and is not permitted. The two-switch topology keeps the non-selected filter isolated.

## Why this architecture

| Candidate | Decision | Reason |
|---|---|---|
| RX5808 only | Rejected | Its published eight channels cover only 5645–5945 MHz; it misses meaningful portions of 5.1–6.0 GHz and cannot characterize modern digital-link energy outside them. |
| Unfiltered AD8318 | Rejected | A broadband log detector cannot establish that energy belongs in 5.x; it is not a spectrum analyzer. |
| Full SDR on motherboard | Rejected | It adds a wideband RF, converter, clocking, host-processing and calibration program unsuitable for the first board. |
| Wi-Fi client receiver | Rejected | It would observe only compatible Wi-Fi behavior, not arbitrary RF energy. |
| Two filtered switched log-detector bins | **Selected** | Real passive 50 Ω paths cover 5.150–5.875 GHz and 5.925–6.000 GHz. Their bins are useful coarse occupancy information, have low MCU cost, and retain a clean path for later SDR modules. The small 5.875–5.925 GHz overlap/gap is observed by the retained RX5808 path, not presented as a calibrated digital detector bin. |

## Exact major components

| Ref | Manufacturer / MPN | Function | Procurement policy |
|---|---|---|---|
| U7 | Analog Devices `AD8318ACPZ-R7` | 1 MHz–8 GHz logarithmic RF detector; one output for power-vs-time observation | Customer-supplied/consigned for the first two PCBA units unless an authorized Chinese distributor can provide traceable reels. No functional substitute without RF calibration and response review. |
| U8, U9 | pSemi `PE42427` | 5 MHz–6 GHz 2×2 mm SPDT input/output filter selection | Customer-supplied/consigned for first article. No lower-frequency "PE4259-like" substitute. |
| FL1 | Johanson Technology `5515BP15B0725001E` | 0805 5.150–5.875 GHz BPF; 1.5 dB typical insertion loss | Authorized-source part; PCBA placement is acceptable after incoming reel/lot review. No substitute without S-parameters and re-test. |
| FL2 | Johanson Technology `6530BP44A1190001E` | 1812 5.925–7.125 GHz BPF; 2.3 dB typical insertion loss; use only 5.925–6.000 GHz | Customer-supplied/consigned. No substitute without checking 5.925–6.000 GHz passband and attenuation. |
| U10 | Texas Instruments `ADS1115IDGSR` | 16-bit I²C ADC for detector and RX5808 RSSI outputs | PCBA source acceptable only from authorized stock; substitution requires I²C/API and reference-voltage review. |
| U11 | Texas Instruments `THS7313PWR` | 3.3 V video buffer; source-terminated VIDEO OUT driver | PCBA source acceptable from authorized stock; do not replace with a generic op amp. |
| U12 | Texas Instruments `LM1881M/NOPB` | Composite-sync separator | Customer-supplied/consigned preferred; 5 V logic output requires the documented 3.3 V translation. |

**Chinese availability:** standard passives, ESP32, Ebyte modules, TI charger/regulators, ADS1115 and LM1881 are generally catalogued through LCSC/JLCPCB-class supply chains but must be checked against the quoted order on the order date. ADI, pSemi and Johanson RF parts are the procurement risk; treat them as consigned first-article components. No availability statement is a guarantee of stock, authenticity or PCBA feeder support.

## Signal contract and limitations

- The 5.x receiver emits a time-stamped relative level and one of two **filter bins**. It does not produce a channel number, modulation, bandwidth, protocol, transmitter identity, direction or distance.
- `FIVE_X_LOW` represents 5.150–5.875 GHz energy through FL1. `FIVE_X_HIGH` represents 5.925–6.000 GHz energy through FL2. The output is calibrated only after a conducted sweep with the fitted antenna/front end and a documented source.
- The ESP32 samples the detector through an external ADC; it does not have to perform RF-rate sampling. Firmware records level, selected bin, timestamp, burst duration, duty cycle and filtered trend.
- RX5808 remains the only analog-video demodulator. Its RSSI is separate evidence; valid composite sync is required before `ANALOG_FPV_CONFIRMED`.
- `DIGITAL_FPV_LIKE` is an evidence-fusion label for contemporaneous 2.4 GHz + 5.x observations. It is deliberately not DJI, Walksnail, ArtLynk or HDZero identification.
- Any brand-specific experimental label requires measured, versioned datasets and an independently reviewed classifier. None is in Rev C.1.

## RF implementation rules

1. J8, U8/U9, FL1/FL2 and U7 are a single edge-zone RF island. Use F.Cu controlled 50 Ω CPWG/microstrip referenced to uninterrupted In1 GND, no RF vias, no test stubs and via fences along the island boundary.
2. Fabricator must field-solve the actual laminate, copper, solder-mask and CPWG geometry. The nominal Rev C 0.18 mm prepreg is not an impedance certificate.
3. Place the RF island away from U1 antenna notch, GPS antenna, switching inductors, microSD, display flex, battery and the 900 MHz receiver. Do not route digital clocks or switch-control lines beneath/through it.
4. The 5.x SMA must have a continuous ground launch and a documented antenna service volume. Never share its antenna with RX5808 through an uncharacterized tee.
5. Measure S11 and selected-path insertion loss with a VNA before detector calibration. Stop if either filter path misses its intended passband, if input return loss is unacceptable for the chosen antenna, or if switching leakage prevents stable bin discrimination.

## Analog-video confirmation criteria

`VIDEO_SYNC` is valid only when the sync separator has observed a stable composite-sync train for at least 500 ms and the current RX5808-selected channel has contemporaneous energy above its calibrated active threshold. Loss of sync for 250 ms clears confirmation. This avoids equating a single RSSI threshold with video validity.

The 75 Ω output connector is a **buffered, source-terminated composite-video output** for an external monitor/test instrument. It is not HDMI and the ESP32 does not capture, decode or render CVBS.
