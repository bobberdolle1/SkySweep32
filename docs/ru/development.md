# Разработка и вклад

SkySweep32 принимает firmware, RF-measurement, mechanical, documentation и
localization вклад. До pull request прочитайте [CONTRIBUTING.md](../../CONTRIBUTING.md).

## Локальные проверки

```bash
make -C test/host
cppcheck --enable=warning --std=c++11 \
  -DBOARD_SKYSWEEP32_REV_C -DPROFILE_PASSIVE_MONITOR \
  -I src -I src/drivers -I src/protocols src
pio run -e esp32s3_rev_c_passive
```

В Windows используйте WSL для sanitizer-backed host suite; см.
[`test/host/README.md`](../../test/host/README.md). Для изменения CAD запустите
`python tools/hardware/verify.py` после реального электрического или механического изменения.

## Дисциплина scope

Не добавляйте active RF interference, claims identity по RSSI или placeholder ML.
Не меняйте Rev C ради внешнего вида до измерений Prototype #1. Сохраняйте чёткую
границу между тем, что собирается, и тем, что доказано на физическом оборудовании.
