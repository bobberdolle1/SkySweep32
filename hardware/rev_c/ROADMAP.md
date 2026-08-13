# Rev C evidence roadmap

**Baseline:** electrical and mechanical architecture is frozen for Prototype #1.
Do not create Rev C.1 or Rev D merely because a CAD rendering could look prettier.
Change Rev C only for a demonstrated electrical, mechanical, manufacturability,
or safety defect.

## Prototype #1 — required before any maturity advance

- Confirm every fitted MPN, polarity, orientation, and assembly outcome.
- Measure USB, battery/charger, 3.3 V and 5 V rails under representative load;
  record thermal behavior and charge safety observations.
- Characterize 855–925 MHz, 2.4 GHz, and 5.8 GHz inputs with documented sources,
  antenna configurations, frequency points, levels, RSSI response and false
  activity behavior. Do not claim sensitivity/selectivity without measurements.
- Verify GNSS, microSD, OLED, buttons, alerts, Wi-Fi Web UI, BLE receive, and
  ESP-NOW. Record range/coexistence instead of assuming it.
- Print the enclosure and prove assembly, USB/battery/SD access, display fit,
  antenna cable routing, fastener/boss clearance, retention, and removal.
- Record raw results and failures with firmware SHA and build provenance.

The target maturity after successful physical work is **PROTOTYPE_ASSEMBLED**,
then **BENCH_TESTED** only for the measured functions. It is not production
validation.

## Possible improvements — evidence required first

- RF: measured antenna matching, receiver dynamic range/selectivity, front-end
  filtering, shielding, isolation, and calibration only after a repeatable test setup exists.
- Power: efficiency, switchable 5 V rail, thermal margins, battery gauge and
  charging behavior after measured load profiles.
- Mechanics: enclosure refinement, harness/antenna service, dimensional changes,
  and optional compact variants after a printed/assembled baseline.
- System: multi-node event correlation only with timestamped test data.

## Explicit future directions

- An external LoRa-class transport may be evaluated after a protocol, power,
  coexistence, regulatory and user requirement are defined. It is not fitted to Rev C.
- 5.8 GHz analog-video/CVBS work requires a distinct documented receive/demodulation experiment.
- TinyML requires a real dataset, reproducible training, model artifact, metrics,
  and physical validation; placeholder bytes are not a feature.
- Mini, Rev D, and production-candidate hardware need measured requirements and
  a separate architecture decision.
