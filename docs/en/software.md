# Firmware, logging, UI, and networking

The product firmware is C++ in `src/`; its canonical PlatformIO environment is
`esp32s3_rev_c_passive`. The hardware manifest generates the pin/profile header
before each build.

## Runtime subsystems

- `main.cpp`: explicit Rev C boot sequence and FreeRTOS tasks.
- `drivers/`: CC1101, SX1281 and RX5808 receive-control paths.
- `activity_classifier.*`: normalized energy/activity presentation; no identity inference.
- `gps_module.*`, `data_logger.*`, `power_manager.*`: GNSS, logs and portable-power state.
- `web_server.*` plus `dashboard.html`: local dashboard/API.
- `espnow_mesh.*`: local SkySweep32 event sharing; range/coexistence need tests.
- `remote_id_detector.*`: retained experimental BLE parser; disabled in the
  canonical build pending defensible multi-transmitter association tests.
- `protocols/`: CRSF and MAVLink bounded parsers. They are not wired proof of over-air protocol reception.

## Logging and web UI

The web server and microSD logger consume local receiver activity and device
state. Logs may contain sensitive location or receiver information; collect,
store, and share them lawfully. The AP password is per-device, generated on
first boot, and is never returned by a GET API. Dashboard/status telemetry is
read-only for AP clients; configuration, power changes, and logs require
`admin` plus that password. Network changes persist and require reboot; Rev C
supports AP mode only. USB is the only Prototype #1 update route—there is no
OTA endpoint.

## Networking boundary

ESP-NOW is the only current node-to-node transport path. It shares coarse events
without claiming source identity. LoRa/Meshtastic compatibility is not implemented
for Rev C; an external transport is a future design decision, not a feature toggle.
