# Валидация и доказательства прототипа

Текущие воспроизводимые design checks Rev C:

```bash
python tools/development/generate_rev_c_pinmap.py --check
python tools/development/generate_dashboard.py --check
python tools/hardware/rev_c/verify_schematic_parity.py
python tools/hardware/verify.py
pio run -e esp32s3_rev_c_passive
make -C test/host
```

Полный hardware gate заново создаёт ERC/DRC reports, CAD models,
interference/service checks корпуса, fabrication outputs и firmware build. Он не
dоказывает antenna response, sensitivity, charging safety, thermal behavior,
USB integrity, GNSS performance, field reliability, compliance или production yield.

## Обязательные доказательства Prototype #1

Следуйте [physical validation checklist](../../hardware/rev_c/PROTOTYPE_VALIDATION_CHECKLIST.md): осмотрите сборку; измерьте rails/charge/thermal; охарактеризуйте все receive paths; проверьте GNSS, microSD, controls, Web UI, BLE/ESP-NOW; подтвердите fit и service корпуса.

Сообщайте реальные результаты через hardware-build issue template. Укажите
ревизию платы, изготовителя/монтаж, BOM substitutions, firmware SHA, setup
измерения, raw logs, фото и отрицательные результаты.
