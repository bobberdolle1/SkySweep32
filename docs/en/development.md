# Development and contributing

SkySweep32 welcomes firmware, RF-measurement, mechanical, documentation, and
localization contributions. Read [CONTRIBUTING.md](../../CONTRIBUTING.md) before
opening a pull request.

## Local checks

```bash
make -C test/host
cppcheck --enable=warning --std=c++11 \
  -DBOARD_SKYSWEEP32_REV_C -DPROFILE_PASSIVE_MONITOR \
  -I src -I src/drivers -I src/protocols src
pio run -e esp32s3_rev_c_passive
```

Use WSL for the sanitizer-backed host suite on Windows; see
[`test/host/README.md`](../../test/host/README.md). CAD contributors use
`python tools/hardware/verify.py` after a genuine electrical or mechanical
change.

## Scope discipline

Do not add active RF interference, source-identification claims from RSSI, or
placeholder ML features. Do not change Rev C for cosmetic reasons before
Prototype #1 measurements. Preserve a clear distinction between source that
builds and behavior verified on physical hardware.
