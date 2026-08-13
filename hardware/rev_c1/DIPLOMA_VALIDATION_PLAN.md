# Rev C.1 detector validation plan

This plan validates observations and documented limits, not marketing claims. Every run records device serial/revision, firmware hash, antenna, power mode, time/GNSS state, geometry, source setup, raw `BandObservation` records and result criteria.

| Experiment | Controlled setup | Expected defensible result | Disallowed conclusion |
|---|---|---|---|
| Ordinary Wi-Fi | Known 2.4/5 GHz Wi-Fi AP, then traffic/no-traffic states | Activity appears in the affected RF bins; raw level/duty changes are logged. | UAV, DJI or drone confirmation from Wi-Fi energy. |
| 900 MHz / 2.4 GHz RC | Known RC/telemetry transmitter, variable packet rate | Activity and temporal/burst features are recorded in its appropriate band. | Protocol identity or aircraft identity from RSSI alone. |
| Analog FPV | Known analog VTX plus PAL/NTSC camera; RF-only/no-video control run | RSSI activity followed by stable composite sync causes `ANALOG_FPV_CONFIRMED`; RF-only/noise does not. | Video content decode, range or pilot identification. |
| Digital FPV | Known digital transmitter on representative 5.x channels | Selected 5.x bin records energy/time features. Correlated 2.4 + 5.x can produce `DIGITAL_FPV_LIKE`. | Video decode, brand identity, occupied channel finer than the filter bin. |
| DJI / Walksnail / ArtLynk / HDZero datasets | Versioned transmitter/firmware/configuration and multiple controlled distances/orientations | Store labeled raw observations for future classifier research. | Any brand-specific classifier result before held-out measured validation. |
| Relative movement | Fixed transmitter; repeat controlled approach, static and retreat trajectories; record geometry | Filtered same-event level changes produce `APPROACHING`, `STABLE` or `RECEDING` where repeatable. | Meter-distance from RSSI. |
| Multi-band correlation | Controlled independent 900, 2.4 and 5.x sources; then synchronized sources | Confidence/evidence flags rise only for time-correlated independent observations. | Causation or aircraft identity merely from simultaneous RF. |

## Reproducibility and acceptance

1. Publish raw CSV/JSON observations alongside source configuration and calibration files; charts are derived evidence.
2. Include null controls: powered detector with no intended emitter, Wi-Fi-only, non-video analog RF/noise, and out-of-bin RF source.
3. Measure receiver false activity over a defined observation duration in the intended enclosure/power/display configuration.
4. For each RF path, preserve calibrated source level or a validated conducted attenuation chain. Do not compare uncalibrated antenna measurements across different configurations as absolute sensitivity.
5. Separate source-level changes from antenna position, polarization, obstruction, multipath, adaptive transmit power and self-interference; each is a confounder for proximity.
6. Remote ID remains disabled unless a dedicated multi-transmitter association implementation passes standards/reference-vector and interleaving tests. BLE/Wi-Fi Remote ID is not proprietary DJI Drone-ID.
