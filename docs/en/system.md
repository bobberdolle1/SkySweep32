# System architecture and capability matrix

```text
RF environment
  └─ fitted passive frontends: 855–925 MHz · 2.4 GHz · 5.8 GHz
       ↓ relative RSSI / activity observations
ESP32-S3 core
  ├─ acquisition and activity processing
  ├─ GNSS timing/position
  ├─ microSD logging
  ├─ OLED, buttons, buzzer and vibration output
  ├─ local Wi-Fi web UI and BLE receive path
  └─ ESP-NOW event sharing
       ↓
OLED · SD logs · local web UI · alerts · networked activity events
```

Optional future directions are node correlation, an external LoRa-class
transport, 5.8 GHz video experiments, and evidence-backed ML. None is a Rev C
production capability.

## Current truth matrix

| Area | Source state | Physical evidence | Public status |
| --- | --- | --- | --- |
| 855–925 MHz | builds; CC1101 activity samples at 860/890/920 MHz | none | requires prototype RF characterization |
| 2.4 GHz | builds; SX1281 RSSI sweep | none | energy/activity only |
| 5.8 GHz | builds; RX5808 channels and analog RSSI | none | incoming module check and bench response required |
| GNSS, microSD, OLED, controls, battery | source + Rev C contract | no board | requires physical bring-up |
| Wi-Fi web UI, BLE receive, ESP-NOW | source builds | no Rev C run/range test | experimental on Rev C |
| Remote ID | BLE parser source | no conformance fixture/hardware test | experimental; not a standards claim |
| MAVLink / CRSF | host-tested bounded parsers | no Rev C demodulated input | parser infrastructure only |
| ATAK, LoRa/Meshtastic, TinyML, 5.8 video | no current product path | none | roadmap only |

SkySweep32 is passive. It does not jam, inject, deny GPS, direction-find, or
identify transmitters from RSSI/activity alone.
