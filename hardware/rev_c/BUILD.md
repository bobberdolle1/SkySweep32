# Build SkySweep32 Rev C

**Status: READY FOR FIRST PHYSICAL PROTOTYPE — NOT PRODUCTION VALIDATED.**

This is the sole fabrication path for Rev C. Do not combine these files with
Rev A, Rev B, a `v0.6.1` binary, or an ESP32 DevKit wiring guide.

## What to order

1. Fabricate `manufacturing/skysweep32_rev_c_gerbers.zip` as a **150 × 95 mm,
   four-layer** board: 1.6 mm finished FR-4, ENIG, 35 µm copper, 0.20 mm
   minimum spacing and drill. Use the stackup in [README.md](README.md).
2. Request 90 Ω USB D+/D− and 50 Ω J5/J7 launches, field-solved against the
   actual fabricator stackup. A supplier geometry change requires a fresh
   impedance and DRC review.
3. Assemble only `manufacturing/bom_fitted.csv` and
   `manufacturing/positions.csv`. `bom.csv` contains intentional DNP items.
4. Order non-PCB items from `manufacturing/assembly_items.csv`. Fit **one** J5
   regional antenna variant, not both.
5. Print the base, lid, and button STLs under `enclosure/`; use the specified
   Adafruit OLED, cable, battery, and internal antenna in the manifest.

`manufacturing/fabrication_manifest.json` records the checked fabrication-file
hashes. It does not certify a supplier output or an assembled board.

## Required checks before purchase

From the repository root, with the pinned tools in `tools/hardware/toolchain.json`:

```bash
python tools/development/generate_rev_c_pinmap.py --check
python tools/development/generate_dashboard.py --check
python tools/hardware/rev_c/verify_schematic_parity.py
python tools/hardware/verify.py
python tools/development/verify_documented_commands.py
python tools/development/verify_prototype1_security.py
```

The first command proves that the human-readable hierarchy preserves the
reviewed pre-hierarchy reference/pin/net contract. The full verifier runs ERC,
DRC, PCBA/enclosure checks, manufacturing export, and the canonical firmware
build. A pass remains CAD/build evidence only.

## Assemble and power safely

Follow [ASSEMBLY_AND_BRINGUP.md](ASSEMBLY_AND_BRINGUP.md) in order:

- inspect the bare board before assembly;
- inspect power-path soldering and RX5808 lot/pinout before fitting it;
- begin at 5.00 V and a 100 mA current limit with battery, antennas, OLED, and
  microSD disconnected;
- stop on a hard short, rail outside 5%, rapid heating, or unexpected current;
- only then test native USB, controls, each peripheral, battery behavior, RF
  response, and enclosure serviceability.

Record results in [PROTOTYPE_VALIDATION_CHECKLIST.md](PROTOTYPE_VALIDATION_CHECKLIST.md).
A completed build may advance maturity only when the corresponding measured
results and raw evidence are reviewed into the repository.

## Firmware

Rev C firmware is source-built only:

```bash
pio run -e esp32s3_rev_c_passive
pio run -e esp32s3_rev_c_passive --target upload
pio device monitor --baud 115200
```

The historical `v0.6.1` binaries target legacy ESP32 DevKit wiring and **must
not** be flashed onto Rev C.
