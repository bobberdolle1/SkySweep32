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

At first boot—or the first boot after a factory reset—the firmware generates a
unique WPA2 AP password, stores it in SPIFFS, and shows the AP SSID/password on
the OLED and USB serial console. Later boots reuse the stored credential.

The dashboard and status telemetry are readable to clients admitted to that AP.
Management configuration, power policy, and SD logs require HTTP Basic
authentication: username `admin`, password = that per-device AP password.
`POST /api/config` persists changes for the next reboot; Rev C currently
supports AP mode only. Remote ID code is retained but **disabled by default**;
it is experimental and not a standards claim. USB is the only Prototype #1
firmware update route; there is no network OTA endpoint.

## Questions and bugs

Ask questions or propose ideas in [GitHub Discussions](https://github.com/bobberdolle1/SkySweep32/discussions). File reproducible defects as [Issues](https://github.com/bobberdolle1/SkySweep32/issues). Use the hardware-build template for physical measurements and photos.
