# Текущая аппаратура: Rev C

**ГОТОВ К ПЕРВОМУ ФИЗИЧЕСКОМУ ПРОТОТИПУ — НЕ ПРОИЗВОДСТВЕННО ПРОВЕРЕН.** Rev C
— текущая физическая реализация системы SkySweep32. Его электрическая и
механическая архитектура заморожена для Prototype #1, кроме доказанного defect.

- ESP32-S3-WROOM-1-N16R8 и native USB-C.
- Пассивные тракты E07/CC1101 855–925 МГц, E28/SX1281 2.4 ГГц, RX5808 5.8 ГГц.
- GNSS SAM-M10Q, microSD, OLED harness, локальные кнопки и alerts.
- USB-C и protected 1S battery path; Wi-Fi/BLE/ESP-NOW через ESP32-S3.
- Четырёхслойная PCB 150 × 95 мм и печатный корпус для помещения.

Точные детали, GPIO, fitted/DNP status, power assumptions и mechanical envelopes
определяет [hardware manifest](../../hardware/rev_c/hardware_manifest.json).
KiCad — источник изготовляемой схемы; документация производителя — реальность компонента.

## Сборка и монтаж

Используйте [BUILD_THIS.md](../../BUILD_THIS.md), затем [build guide](../../hardware/rev_c/BUILD.md) и [assembly/bring-up](../../hardware/rev_c/ASSEMBLY_AND_BRINGUP.md). Используйте только fitted BOM и fabrication package Rev C.

Корпус CAD-проверен с PCBA, батареей, дисплеем, антеннами, harness и service
envelopes. Он для помещения, не герметичен, без IP-rating и всё ещё требует
первой печати и реальной сборки.

Rev A и Rev B отсутствуют из пути сборки default branch. История остаётся в
[истории ревизий](../history/hardware-revisions.md).
