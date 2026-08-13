# Rev C.1 native KiCad 10.0.5 check record

**Status: NO-GO.** These checks establish the current native-source state; they do not substitute for the missing Rev C.1 circuit/layout work.

| Check | Command/result | Disposition |
|---|---|---|
| Schematic ERC | `kicad-cli sch erc --exit-code-violations ... skysweep32_rev_c1.kicad_sch` | **PASS — 0 violations.** The Rev C.1 `fp-lib-table` was corrected to reference `SkySweep32RevC1.pretty`; the prior five library warnings therefore no longer mask ERC. |
| PCB DRC | `kicad-cli pcb drc --exit-code-violations ... skysweep32_rev_c1.kicad_pcb` | **FAIL — inherited board has 11 violations and 7 unconnected items.** Includes antenna-notch edge-clearance conflict, disconnected GND copper, dangling GND vias and silkscreen clipped by the notch. The parent Rev C PCB also reports seven violations/seven unconnected items under the same KiCad version; Rev C.1 adds the notch-derived conflicts. No DRC-clean claim is permitted. |
| Schematic/PCB net parity | `python tools/hardware/rev_c1/verify_schematic_pcb_parity.py` | **PASS — 357 fitted PCB pads match the exported native schematic netlist.** This checks only the inherited source contract; it does not validate missing Rev C.1 circuits, placement, routing, impedance, or RF performance. |
| Board render | `kicad-cli pcb render ...` | Completed. The inspected render confirms the new top-edge notch but also confirms that the PCB is still the Rev C placement/routing with no 5.x detector, video, SkyPort or compacted regulator clusters. |
| Firmware contract | `python tools/development/generate_rev_c1_pinmap.py --check` | **PASS.** The declared detector, analog-video and SkyPort GPIO allocation is unique and excludes ESP32-S3 reserved pins. |
| Firmware build | `pio run -e esp32s3_rev_c1_detector` | **PASS — 9.68 s after the final observation-log/dashboard update.** This proves the generated board/profile contract and current firmware compile together. It is not evidence that the pre-fabrication PCB implements those signals. |

## Key evidence

KiCad 10.0.5 was installed per-user at `C:\Users\kiril\AppData\Local\Programs\KiCad\10.0\bin\kicad-cli.exe`. The unmodified native PCB remains the physical evidence: U5/L2 and U3/L1 spacing, USB topology, RX5808 pin 11 no-connect, and absent new circuitry remain release blockers documented in `../REV_C1_PREFAB_REVIEW.md`.

Do not suppress or delete DRC findings to produce a green report. Correct every applicable native design defect, re-run ERC/DRC, and regenerate this record from the resulting source.
