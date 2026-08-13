# Validation and prototype evidence

Rev C's current reproducible design checks are:

```bash
python tools/development/generate_rev_c_pinmap.py --check
python tools/development/generate_dashboard.py --check
python tools/hardware/rev_c/verify_schematic_parity.py
python tools/hardware/verify.py
pio run -e esp32s3_rev_c_passive
make -C test/host
```

The complete hardware gate rebuilds ERC/DRC reports, CAD models, enclosure
interference/service checks, fabrication outputs and the firmware build. It does
not prove antenna response, receiver sensitivity, charging safety, thermals,
USB integrity, GNSS performance, field reliability, compliance, or production
yield.

## Required Prototype #1 evidence

Follow the [physical validation checklist](../../hardware/rev_c/PROTOTYPE_VALIDATION_CHECKLIST.md): inspect assembly; measure rails/charge/thermal behavior; characterize all receive paths; test GNSS, microSD, controls, web UI, BLE/ESP-NOW; and prove enclosure/service fit.

Report real outcomes through the hardware-build issue template. Include board
revision, fabrication/assembly provenance, BOM substitutions, firmware SHA,
measurement setup, raw logs, photographs, and negative results.
