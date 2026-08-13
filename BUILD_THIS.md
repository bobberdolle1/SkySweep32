# Build the current SkySweep32

This is the short route for the only current hardware/firmware combination:
**SkySweep32 Rev C passive monitor**.

> **READY FOR FIRST PHYSICAL PROTOTYPE — NOT PRODUCTION VALIDATED.** Reproduce
> the checks below before fabrication. A successful check does not replace
> Prototype #1 electrical, RF, mechanical, or compliance evidence.

## 1. Inspect the exact hardware package

Read [`hardware/rev_c/BUILD.md`](hardware/rev_c/BUILD.md). It is the canonical
order/assembly route, including the fitted BOM, Gerbers, drill data, assembly
items, enclosure, bringing-up procedure, and physical validation checklist.

Do not use Rev A/Rev B assets or the historical `v0.6.1` binaries.

## 2. Reproduce design evidence

```bash
python tools/development/generate_rev_c_pinmap.py --check
python tools/development/generate_dashboard.py --check
python tools/hardware/rev_c/verify_schematic_parity.py
python tools/hardware/verify.py
```

The last command requires KiCad 10 and FreeCAD. It recreates engineering
artifacts from native sources; it is not a physical validation claim.

## 3. Build firmware

```bash
pio run -e esp32s3_rev_c_passive
make -C test/host
```

After an actual assembled-board bring-up, upload with:

```bash
pio run -e esp32s3_rev_c_passive --target upload
```

## 4. Record prototype evidence

Run every applicable item in
[`hardware/rev_c/PROTOTYPE_VALIDATION_CHECKLIST.md`](hardware/rev_c/PROTOTYPE_VALIDATION_CHECKLIST.md).
Record measurements, setup, raw logs, photographs, firmware SHA, and failures in
the hardware-build issue template. Do not relabel Rev C as production-validated
until that evidence exists.

For architecture, features, and software, start at [`docs/en/README.md`](docs/en/README.md) or [`docs/ru/README.md`](docs/ru/README.md).
