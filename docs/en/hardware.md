# Current hardware: Rev C

**READY FOR FIRST PHYSICAL PROTOTYPE — NOT PRODUCTION VALIDATED.** Rev C is the
current physical implementation of the SkySweep32 system. Its electrical and
mechanical architecture is frozen for Prototype #1 unless a demonstrated defect
requires a fix.

- ESP32-S3-WROOM-1-N16R8; native USB-C programming.
- Passive 855–925 MHz E07/CC1101, 2.4 GHz E28/SX1281, and 5.8 GHz RX5808 RSSI paths.
- SAM-M10Q GNSS, microSD, OLED harness, local buttons and alerts.
- USB-C and protected 1S battery path; Wi-Fi/BLE/ESP-NOW via ESP32-S3.
- Four-layer 150 × 95 mm PCB and printed indoor enclosure.

The exact authoritative parts, pin map, fitted/DNP status, power assumptions,
and mechanical envelopes are in the [hardware manifest](../../hardware/rev_c/hardware_manifest.json).
KiCad source defines the manufactured circuit; manufacturer documentation defines
component reality.

## Build and assembly

Use [BUILD_THIS.md](../../BUILD_THIS.md), then the Rev C [build guide](../../hardware/rev_c/BUILD.md) and [assembly/bring-up procedure](../../hardware/rev_c/ASSEMBLY_AND_BRINGUP.md). Use only its fitted BOM and fabrication package.

The enclosure is CAD-checked around the PCBA, battery, display, antennas,
harnesses and service envelopes. It is indoor, not sealed, not IP-rated, and
still needs a first printed-case / physical-assembly check.

Rev A and Rev B are not present in the default-tree build path. Their history is
preserved in [hardware revision history](../history/hardware-revisions.md).
