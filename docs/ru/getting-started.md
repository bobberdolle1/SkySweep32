# Начало работы

## Прошивка

Установите PlatformIO, клонируйте репозиторий и соберите единственную активную цель:

```bash
pio run -e esp32s3_rev_c_passive
pio run -e esp32s3_rev_c_passive --target upload
```

**Не прошивайте Rev C** файлами `v0.6.1` с GitHub. Это исторические binary для
другой разводки ESP32 DevKit. Готового Rev C binary release пока нет.

## Аппаратная часть

Откройте корневой [маршрут сборки](../../BUILD_THIS.md), затем точное
[руководство Rev C](../../hardware/rev_c/BUILD.md). Там есть BOM, Gerber,
assembly items, корпус, bring-up и чек-лист физических испытаний.

Rev C — пакет инженерного прототипа, не серийный заказ. До закупки воспроизведите
документированные design checks. После сборки зафиксируйте измерения питания, RF,
GNSS, storage, UI, сети и корпуса.

## Локальный Web UI

После успешного физического запуска Wi-Fi прошивка поднимает локальную AP и
печатает адрес через USB serial. Dashboard показывает активность приёмников,
состояние устройства и экспериментальные сообщения Remote ID. Он не доказывает
идентичность передатчика или соответствие Remote ID стандарту.

## Вопросы и ошибки

Вопросы и идеи — в [GitHub Discussions](https://github.com/bobberdolle1/SkySweep32/discussions). Воспроизводимые defects — в [Issues](https://github.com/bobberdolle1/SkySweep32/issues). Для измерений и фото прототипа используйте hardware-build template.
