# Rev C.1 preliminary power budget

**Status: engineering allocation, not measured data.** The existing Rev C 1.35 A / 3.3 V aggregate number cannot be reused because Rev C.1 adds an ADC, switched 5.x detector, analog-video chain and SkyPort allocation.

## Rail allocation

| Load | 3.3 V nominal / peak | 5 V nominal / peak | Basis |
|---|---:|---:|---|
| ESP32-S3-WROOM-1-N16R8 | 180 / 500 mA | — | Radio/CPU peak must preserve Espressif's ≥500 mA supply recommendation. |
| E28/SX1281 2.4 GHz path | 35 / 100 mA | — | Module activity/sweep allowance. |
| E07/CC1101 855–925 MHz path | 25 / 50 mA | — | Module activity/sweep allowance. |
| SAM-M10Q GNSS | 35 / 100 mA | — | Acquisition allowance. |
| microSD | 30 / 200 mA | — | Write burst allowance. |
| OLED, controls, gauge, LEDs, I²C | 45 / 80 mA | — | Enabled UI allowance. |
| 5.x AD8318 + two PE42427 + ADC/expander | 18 / 30 mA | — | Detector active; value requires measured verification. |
| SkyPort 3.3 V allocation | 0 / 100 mA | — | Switched; granted only after descriptor check. |
| RX5808 exact module | — | 170 / 180 mA | Legacy module data sheet maximum current. |
| THS7313 / LM1881 / sync translator | — | 12 / 20 mA | Video buffer and separator allocation. |
| SkyPort 5 V allocation | 0 / 100 mA | — | Switched, hard-limited total. |
| 5 V miscellaneous / margin | — | 20 / 25 mA | Switch, losses and local margin. |
| **Total** | **368 / 1160 mA** | **202 / 325 mA** | Peak assumes every enabled function and maximum permitted SkyPort load. |

At the peak allocation the 3.3 V buck output is **3.83 W** and direct 5 V load is **1.63 W**. Accounting for AP63203 and TPS61232 losses produces an estimated 5 V boost load of roughly **1.2 A / 6.0 W**. At a 3.4 V battery and a conservative 85% boost efficiency, the battery input is roughly **2.1 A** before battery protection, cable, temperature and aging derating.

## Consequences

- The `TPS61232DRCR`, L2 saturation/DC resistance, BQ24074 system path, 1S pack protection FETs, JST-PH harness and PCB temperature must be validated at the peak allocation. IC headline current ratings are not sufficient.
- The BQ24074 is configured around a 1.3 A USB input limit and 800 mA charge setting. A 6 W peak system load leaves no charge margin on a 5 V/1.3 A source; battery supplement or charge-current reduction is expected. Charging plus peak detector operation is therefore a **first-article measurement and stop-condition item**, not a claim.
- The 100 mA SkyPort allocation is deliberate. It keeps a low-cost sensor/module class possible without falsely promising SDR-class power from a compact 1S boost rail.
- First article must measure: cold boot inrush; USB-only peak; battery-only peak; charge plus active scan; 5 V and 3.3 V ripple/droop at each load state; all regulator/inductor/battery connector temperatures at 5, 15 and 30 minutes; and recovery after connector hot-plug.
- Stop before RF work if 3.3 V or 5 V is outside ±5%, a regulator reaches its thermal limit, a current-limit cycles, the battery connector exceeds its reviewed temperature, or charging fails to regulate safely under active load.
