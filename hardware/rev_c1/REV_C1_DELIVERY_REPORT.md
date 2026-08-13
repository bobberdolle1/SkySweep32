# Rev C.1 Detector delivery report

## Recommendation: **NO-GO — do not fabricate Rev C.1**

This is the correct conservative result. The independent review found real pre-fabrication defects, and this workstation lacks the KiCad 10 CLI, FreeCAD and C++/PlatformIO toolchains required to safely complete, electrically validate and regenerate a fabrication package. It would be unsafe to represent a renamed or hand-edited Rev C board as the requested first physical hardware.

## Work completed

1. **Independent forensics:** inspected Rev C's five native hierarchy sheets, PCB stackup, footprints, fitted BOM, placement, Gerbers/manifest, firmware pin contract, RX5808 driver, enclosure validation and Remote ID default-disabled state.
2. **Review findings:** the complete confirm/reject/partial table is in `REV_C1_PREFAB_REVIEW.md`.
   - QSTRT-to-GND finding rejected: QSTRT is an active-low quick-start input; NC is the correct unused electrical state. The generic U6 symbol is still a real documentation/ERC defect.
   - TPS61232 cluster, AP63203 cluster and USB pair defects are confirmed by exact native-board coordinates/routes.
   - ESP32 copper keepout exists, but it was laminate-backed and enclosure clearance was not proven. Rev C.1 PCB source adds the x=48–96 mm, y=0–22.25 mm board-edge antenna notch; enclosure derivation remains blocked.
   - RX5808 12-pad geometry/pin map match the referenced legacy drawing, but VIDEO was NC and lot qualification does not exist.
3. **Detector architecture:** `DETECTOR_ARCHITECTURE.md` defines a technically bounded two-bin 5.x path: paired 6 GHz PE42427 switches, Johanson 5.150–5.875 and 5.925–6.000 GHz filters, AD8318 log detector and ADS1115. RX5808 remains the analog-video receiver. This produces coarse activity/temporal evidence, not a spectrum analyzer, video decoder, brand ID or distance meter.
4. **Analog-video plan:** buffered, 75 Ω source-terminated VIDEO OUT plus LM1881 composite sync output. Firmware rule: only a stable 500 ms sync train with current-channel energy may emit `ANALOG_FPV_CONFIRMED`.
5. **Expansion contract:** `SKYPORT.md` defines a keyed JST-GH 16-pin interface, 5 V/3.3 V 100 mA limits, protected sequencing, I²C/SPI/UART/GPIO, EEPROM descriptor and passive-only discovery contract.
6. **Evidence-fusion implementation:** added `src/detection_model.h/.cpp`, host test cases and live scan-loop integration. The fixed-storage model publishes `BandObservation`, `SignalFeatures`, `DetectionEvidence` and `Detection`, confines classifications to the allowed early set, records flags/confidence/trend and has no brand-specific classification. Existing logs distinguish uncorrelated `energy` from `correlated` evidence; a Rev C.1 raw-record schema remains a release blocker.
7. **Power, cost and validation plans:** documented in `POWER_BUDGET.md`, `PROCUREMENT_AND_COST.md`, `FIRST_ARTICLE_BRINGUP.md` and `DIPLOMA_VALIDATION_PLAN.md`.

## Required changes relative to Rev C

| Area | Required Rev C.1 change | Why |
|---|---|---|
| U5 boost | Compact U5/L2/C20/C21 cluster and new short SW loop | Existing 10 mm U5/L2 spacing and ~15 mm SW trace violate TI layout intent. |
| U3 buck | Compact U3/L1/input/output/feedback cluster | Existing inductor and capacitors are physically remote. |
| USB | New F.Cu 90 Ω pair above solid In1 GND, no signal vias | Existing route is unequal/separated and runs over In2 power. |
| ESP32 antenna | Native board-edge notch plus new PCBA-derived enclosure RF cavity | Rev C only cleared copper, leaving base laminate and unproven enclosure clearance. |
| U6 | Native MAX17048 symbol with explicit QSTRT/ALRT NC | Generic connector symbol defeats meaningful review. |
| RX5808 | Qualified consigned lot and pin-11 VIDEO circuit | Existing board has no analog-video validity path. |
| 5.x | Two switched filtered detector bins and ADC | RX5808 alone cannot observe 5.1–6.0 GHz digital-link activity. |
| SkyPort | Protected fixed interface and module descriptor | Prevents unbounded current/unversioned GPIO expansion. |
| Power/enclosure | Recalculate, measure and regenerate from the final PCBA | Existing allocation/enclosure cannot establish Rev C.1 behavior. |

## Verification evidence

- **Passed static checks:** Rev C.1 manifest parses as JSON. Parenthesis balance is zero for its PCB and all five schematic files. New detector model contains the fusion contract and no brand labels.
- **Passed firmware build:** `pio run -e esp32s3_rev_c_passive` completed successfully after scan-loop integration in 9.72 seconds (a preceding full build completed in 33.70 seconds). It compiles the changed detector model and its caller with the existing Rev C passive target; it does not validate an unimplemented Rev C.1 pin contract or hardware.
- **Not run / unavailable:** KiCad ERC/DRC/parity/exports, Gerber/drill/BOM/CPL/PDF/STEP/enclosure generation, USB field solve/coupon, host ASan/UBSan test binary and cppcheck. `kicad-cli`, `FreeCADCmd`, `make`, `g++`, `clang++` and `cppcheck` are absent on this workstation.
- **No manufacturing outputs are supplied.** Copying Rev C Gerbers, positions, STEP or enclosure into Rev C.1 would be false evidence because they do not embody the needed circuit/layout changes.

## Remaining uncertainties and release blockers

1. Implement every planned circuit in native KiCad, including real symbols, footprints, BOM entries, placement and routing; do not use the documentation as a substitute.
2. Re-place/re-route both regulator clusters, then run KiCad 10 ERC/DRC and scope/thermal tests.
3. Implement and field-solve USB/50 Ω RF geometry on the selected fabricator stackup; require coupon results.
4. Complete 5.x RF island layout and verify switch/filter selected-path response with VNA before detector calibration.
5. Complete video buffer/sync path and validate with PAL, NTSC, RF-only/no-video and noise controls.
6. Derive a new PCBA STEP and enclosure; validate the antenna cavity, all I/O access and service loops.
7. Regenerate pin contract/board profile and integrate/build the evidence model, raw logger and alarm-first UI.
8. Obtain traceable ADI/pSemi/Johanson/RX5808 lots and first-article PCBA capability confirmation.
9. Generate current BOM/fitted BOM/CPL/Gerbers/drill/assembly drawings/PDF/STEP only after all prior gates pass.

The current source is not production validated and must not be described as a ready first article.
