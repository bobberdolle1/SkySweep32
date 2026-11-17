# Rev C.1 native KiCad 10.0.5 check record

**Status: NO-GO (Work-in-Progress).** The inherited 5-sheet PCB area has been reworked to zero DRC violations and schematic sheets `06`/`07` have been authored, but forward-annotation/layout of sheets `06`–`07`, cross-sheet video signal ties, and I²C manifest reconciliation remain incomplete.

| Check | Command/result | Disposition |
|---|---|---|
| Schematic ERC (7-sheet full hierarchy) | `kicad-cli sch erc ... skysweep32_rev_c1.kicad_sch` → `validation/rev_c1_full_erc.rpt` | **0 Errors, 1 Warning (`footprint_filter` on `J9`).** Includes new `06_detector_video.kicad_sch` and `07_skyport.kicad_sch` plus `SkySweep32RevC1.kicad_sym`. Note: cross-sheet ties `RX5808_VIDEO_RAW` (to sheet `04` RX5808 pin 11) and `VIDEO_SYNC` (to sheet `02` MCU GPIO) are not yet wired. |
| PCB DRC (5-sheet reworked board) | `kicad-cli pcb drc ... skysweep32_rev_c1.kicad_pcb` → `validation/rev_c1_pcb_drc.txt` (`2026-08-14T12:43:35`) | **PASS on 5-sheet inherited board — 0 DRC violations, 0 unconnected pads, 0 footprint errors.** `U5` boost and `U3` buck clusters compacted, `USB_D_P`/`USB_D_N` rerouted on `F.Cu` over solid `In1.Cu` GND, and `U1` normalized to `SkySweep32RevC1:ESP32-S3-WROOM-1_AntennaNotch`. |
| Schematic/PCB parity | `python -B tools/hardware/rev_c1/verify_schematic_pcb_parity.py` | **PENDING FORWARD-ANNOTATION (NO-GO).** The 62 new components on sheets `06` (`U8`–`U12`, `FL1`–`FL2`, `J8`–`J9`, passives) and `07` (`U13`–`U19`, `J10`, passives) are not yet placed or routed on `skysweep32_rev_c1.kicad_pcb`. |
| Firmware/manifest contract | `hardware/rev_c1/hardware_manifest.json` vs sheet `07` (`TCA9534` + `ADS1015`) | **NEEDS RECONCILIATION (NO-GO).** `hardware_manifest.json` still lists placeholder ESP32-S3 GPIOs `22`–`32` (silicon-reserved/internal flash pins on `ESP32-S3-WROOM-1-N16R8`), whereas sheet `07` moves SkyPort/detector control and ADC onto I²C (`TCA9534PWR` `U13` + `ADS1015IDGSR` `U14`); filter/buffer/connector MPNs in `DETECTOR_ARCHITECTURE.md` and `SKYPORT.md` also require alignment with sheets `06`–`07`. |

## Key blockers before fabrication

1. Wire `RX5808_VIDEO_RAW` on `04_rf_5g8.kicad_sch` (RX5808 pin 11) and `VIDEO_SYNC` + `IOX_INT` on `02_control_and_alerts.kicad_sch` to valid ESP32-S3 GPIOs, and clear or document the `J9` `footprint_filter` warning.
2. Reconcile `hardware_manifest.json`, `DETECTOR_ARCHITECTURE.md`, and `SKYPORT.md` with the I²C expander (`TCA9534`), ADC (`ADS1015`), filter (`5235BP44A0180` / `5425BP15A1050`), buffer (`OPA360`), and SkyPort header (`FTSH-108`) design, removing silicon-invalid GPIOs `22`–`32`.
3. Forward-annotate, place, and route all 62 components from sheets `06`–`07` onto `skysweep32_rev_c1.kicad_pcb`, verify 50 Ω RF island geometry, re-run full ERC + DRC + parity, and derive the Rev C.1 PCBA STEP and enclosure.
