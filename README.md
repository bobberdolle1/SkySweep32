# SkySweep32

[![Rev C hardware gates](https://github.com/bobberdolle1/SkySweep32/actions/workflows/hardware.yml/badge.svg)](https://github.com/bobberdolle1/SkySweep32/actions/workflows/hardware.yml)
[![Firmware CI](https://github.com/bobberdolle1/SkySweep32/actions/workflows/platformio.yml/badge.svg)](https://github.com/bobberdolle1/SkySweep32/actions/workflows/platformio.yml)

**Open-source passive multi-band RF monitoring system** — 855–925 MHz, 2.4 GHz,
and 5.8 GHz energy/activity observation; GNSS; microSD logging; local display,
web UI, and networked experiments.

> **Current maturity: READY FOR FIRST PHYSICAL PROTOTYPE — NOT PRODUCTION VALIDATED.**
> Rev C is the current hardware. No Rev C board has been physically assembled,
> bench-tested, RF-characterized, compliance-tested, or field-tested.

SkySweep32 is a **system**, not merely a PCB: passive RF frontends feed an
ESP32-S3 acquisition core; the core drives local logging, GNSS, UI, alerts, web
access, and experimental node events. It does not jam, inject RF, deny GPS,
identify transmitters from RSSI, or provide directional finding.

## Quick links

| Start here | What it answers |
| --- | --- |
| [Get started](docs/en/getting-started.md) · [Начало работы](docs/ru/getting-started.md) | Build the only active firmware target; avoid historical assets |
| [Build Rev C](BUILD_THIS.md) | Exact manufacturing, assembly, and prototype-evidence route |
| [System](docs/en/system.md) · [Система](docs/ru/system.md) | Implemented, experimental, and roadmap capabilities |
| [Current hardware](docs/en/hardware.md) · [Аппаратура](docs/ru/hardware.md) | Rev C PCB, enclosure, manifest, and boundaries |
| [Software](docs/en/software.md) · [Прошивка](docs/ru/software.md) | RF monitoring, logging, UI, protocol parsers, networking |
| [Validation](docs/en/validation.md) · [Валидация](docs/ru/validation.md) | CAD/build evidence versus physical proof |
| [Roadmap](docs/en/roadmap.md) · [План](docs/ru/roadmap.md) | Evidence-driven next work |

## Current system

```text
855–925 MHz / 2.4 GHz / 5.8 GHz passive inputs
             ↓ relative energy/activity
        ESP32-S3 Rev C core
  ┌──────────┼─────────────────────────────┐
  GNSS   microSD logs   OLED/alerts   local Web UI
                    └── ESP-NOW experiments
```

| Capability | Current truth |
| --- | --- |
| Three passive RF observation paths | Implemented in source and Rev C design; RF performance awaits Prototype #1 |
| GNSS, SD logging, OLED, buttons, power telemetry | Implemented in source/design; needs first-board bring-up |
| Wi-Fi web UI, BLE receive, ESP-NOW | Builds; experimental on unassembled Rev C |
| Remote ID, MAVLink, CRSF | Parser/receive infrastructure; no standards or over-air claim |
| LoRa/Meshtastic, TinyML, ATAK, 5.8 video | Not current Rev C capabilities; roadmap only |

## Build and validate

```bash
pio run -e esp32s3_rev_c_passive
make -C test/host
python tools/hardware/verify.py
```

The first two commands exercise source contracts; the hardware command rebuilds
CAD/fabrication checks when its toolchain is available. None proves physical
performance. Follow the [Prototype #1 checklist](hardware/rev_c/PROTOTYPE_VALIDATION_CHECKLIST.md) after assembly.

## Project structure

- `src/` — ESP32-S3 firmware and passive-monitor drivers.
- `hardware/rev_c/` — current canonical KiCad, manifest, fabrication, enclosure, and evidence.
- `tools/hardware/` — reproducible Rev C engineering tools.
- `test/host/` — bounded protocol/parser tests.
- `docs/en/`, `docs/ru/` — product, build, software, validation, and roadmap docs.
- `docs/history/` — deliberately non-orderable Rev A/Rev B context.

## Community and legal boundary

Use [Discussions](https://github.com/bobberdolle1/SkySweep32/discussions) for questions and ideas; use [Issues](https://github.com/bobberdolle1/SkySweep32/issues) for reproducible bugs and Prototype #1 evidence. Operate only under applicable spectrum, privacy, aviation, and data laws. Read [SECURITY.md](.github/SECURITY.md), [CONTRIBUTING.md](CONTRIBUTING.md), and the [GPL-3.0 license](LICENSE).
