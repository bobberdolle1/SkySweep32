# Прошивка, журналы, UI и сеть

Продуктовая прошивка — C++ в `src/`; её каноническая цель PlatformIO —
`esp32s3_rev_c_passive`. Hardware manifest генерирует pin/profile header до
каждой сборки.

## Подсистемы runtime

- `main.cpp`: явный старт Rev C и FreeRTOS tasks.
- `drivers/`: управление приёмными трактами CC1101, SX1281 и RX5808.
- `activity_classifier.*`: нормализованная энергия/activity без identity inference.
- `gps_module.*`, `data_logger.*`, `power_manager.*`: GNSS, логи и portable power state.
- `web_server.*` и `dashboard.html`: локальный dashboard/API.
- `espnow_mesh.*`: обмен событиями между узлами; дальность/coexistence не измерены.
- `remote_id_detector.*`: экспериментальный BLE receive/parser, не ASTM/ASD-STAN conformance.
- `protocols/`: bounded parser CRSF и MAVLink; они не доказывают over-air reception.

## Журналы и Web UI

Web server и microSD logger используют локальную активность приёмников и
состояние устройства. Логи могут содержать чувствительные location/receiver
данные; собирайте, храните и публикуйте их законно. OTA и config endpoints
используйте только в доверенной локальной сети после физического запуска.

## Граница networking

ESP-NOW — единственный текущий node-to-node transport. Он отправляет coarse
события без заявления об identity. LoRa/Meshtastic не реализован для Rev C;
внешний transport — будущая design decision, не feature toggle.
