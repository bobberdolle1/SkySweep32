# Get started

## Firmware

Install PlatformIO, clone the repository, then build the only active target:

```bash
pio run -e esp32s3_rev_c_passive
pio run -e esp32s3_rev_c_passive --target upload
```

Do **not** flash the GitHub `v0.6.1` assets onto Rev C. Those historical binaries
used unrelated ESP32 DevKit wiring. No Rev C binary release exists yet.

## Hardware

Use the root [build route](../../BUILD_THIS.md) and the exact Rev C
[build guide](../../hardware/rev_c/BUILD.md). It links the fitted BOM, Gerbers,
assembly items, enclosure, bring-up procedure, and physical validation checklist.

Rev C is an engineering prototype package, not a production order. Before any
purchase, reproduce the documented design checks. After assembly, record the
required power, RF, GNSS, storage, display, network, and enclosure evidence.

## Local web UI

After a successful physical Wi-Fi bring-up, the firmware starts its local AP and
prints its address over USB serial. The dashboard reports receiver activity,
local state, and experimental Remote ID reports. It does not confer transmitter
identity or Remote ID conformance.

## Questions and bugs

Ask questions or propose ideas in [GitHub Discussions](https://github.com/bobberdolle1/SkySweep32/discussions). File reproducible defects as [Issues](https://github.com/bobberdolle1/SkySweep32/issues). Use the hardware-build template for physical measurements and photos.
