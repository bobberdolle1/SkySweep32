# SkySweep32 — Lua Scripts для EdgeTX / OpenTX

## Установка

### Структура файлов на SD-карте пульта

```
SD/
├── SCRIPTS/
│   ├── TOOLS/
│   │   └── skysweep.lua          ← Основной скрипт (Tools меню)
│   ├── MIXES/
│   │   └── skysweep_bg.lua       ← Фоновый скрипт (непрерыв. мониторинг)
│   └── TELEMETRY/
│       └── skysweep_t.lua        ← Телеметрия (экран при полёте)
└── WIDGETS/
    └── SkySweep/
        └── main.lua              ← Виджет (цветные экраны)
```

### Шаг 1: Скопировать файлы

Подключите пульт к ПК через USB (SD card mode) и скопируйте файлы из
папки `lua/` проекта в корень SD-карты, сохраняя структуру папок.

### Шаг 2: Настроить модель

**Обязательно:** модель должна быть настроена на CRSF протокол:
- `Model → Setup → Internal RF → CRSF` или `External RF → CRSF`

**Для фонового мониторинга:**
- `Model → Custom Scripts → (пустой слот) → skysweep_bg`

**Для телеметрии:**
- `Model → Telemetry → Screen → Script → skysweep_t`

### Шаг 3: Обнаружить сенсоры

`Model → Telemetry → Discover new sensors` — дождаться обнаружения:
- `1RSS`, `2RSS`, `RQly`, `RSNR`, `ANT`, `RFMD`, `TPWR`

---

## Описание скриптов

### `skysweep.lua` — Основной (Tools)

Запуск: `SYS → Tools → SkySweep`

| Страница | Кнопка | Содержание |
|----------|--------|------------|
| RADAR    | ← →   | Радар с анимацией, RSSI, LQ, JAM |
| LINK     | ← →   | Детальная телеметрия, GPS, устройство |
| SPECTRUM | ← →   | График RSSI обеих антенн |
| ALERTS   | ← →   | Лог детекций и аномалий |

**ENTER** — калибровка (замер базовой линии SNR/LQ на чистом эфире)
**EXIT** — выход

### `skysweep_bg.lua` — Фоновый (Mixer)

Работает непрерывно, даже когда основной скрипт закрыт. Функции:
- Отслеживание LQ/SNR/RSSI
- Вычисление Jamming Score (сохраняется в GV9)
- Звуковые алерты при высоком уровне помех

### `skysweep_t.lua` — Телеметрия

Доступен как экран телеметрии (кнопка PAGE при полёте). Две страницы:
- Overview: мини-радар + ключевые метрики
- Graph: график RSSI обеих антенн + LQ/JAM бары

### `SkySweep/main.lua` — Виджет (Color)

Для цветных экранов. Три режима (настройка Mode в параметрах виджета):
- **0 = Radar** — полноцветный rad с анимацией sweep
- **1 = Bars** — барграфы RSSI/LQ/JAM
- **2 = Compact** — минимальный для маленьких зон

---

## Алгоритм детекции помех

Jamming Score (0-100) вычисляется по 6 факторам:

| # | Фактор | Макс. вклад | Описание |
|---|--------|-------------|----------|
| 1 | LQ drop | 40 | Link Quality ниже 70% |
| 2 | SNR degradation | 30 | SNR ниже базовой линии |
| 3 | LQ variance | 15 | Нестабильность Quality |
| 4 | Antenna switching | 10 | Частые переключения антенн |
| 5 | RSSI stable + LQ low | 15 | Паттерн узкополосного глушения |
| 6 | Both antennas degraded | 10 | Широкополосное глушение |

**Калибровка:** нажмите ENTER в чистом эфире для замера baseline.
После калибровки детекция SNR-аномалий значительно точнее.

---

## Совместимость

| Пульт | Экран | Поддержка |
|-------|-------|-----------|
| QX7, QX7S | 128×64 mono | ✅ Tools, Telem, BG |
| X9D, X9D+ | 212×64 mono | ✅ Tools, Telem, BG |
| X9 Lite | 128×64 mono | ✅ Tools, Telem, BG |
| TX16S, TX16S mkII | 480×272 color | ✅ Все + Widget |
| RadioMaster Boxer | 480×272 color | ✅ Все + Widget |
| RadioMaster Zorro | 128×64 mono | ✅ Tools, Telem, BG |
| RadioMaster Pocket | 128×64 mono | ✅ Tools, Telem, BG |
| Jumper T-Pro | 128×64 mono | ✅ Tools, Telem, BG |
| Jumper T-Lite | 128×64 mono | ✅ Tools, Telem, BG |
| FrSky Horus X10/X12 | 480×272 color | ✅ Все + Widget |
| BetaFPV LiteRadio 3 Pro | 128×64 mono | ✅ Tools, Telem, BG |

**Dual-band / True Diversity** (TX16S GX12, etc.): скрипт автоматически
читает `1RSS` и `2RSS` от обеих антенн ресивера. Для dual-band TX-модулей
данные обоих модулей отображаются через стандартную CRSF телеметрию.

---

## Лицензия

GNU GPL v3.0 — как основной проект SkySweep32.
