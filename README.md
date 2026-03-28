# SkySweep32 | Пассивный детектор БПЛА

**Multi-band passive drone detector | Мультидиапазонный пассивный детектор дронов**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Build: PlatformIO](https://img.shields.io/badge/Build-PlatformIO-orange.svg)](https://platformio.org/)

[English](#english) | [Русский](#russian)

---

<a name="english"></a>
## 🇬🇧 English

### Overview

SkySweep32 is an advanced passive drone detection system based on the ESP32 microcontroller. It monitors radio spectrum across three frequency bands (900 MHz, 2.4 GHz, 5.8 GHz) to detect UAV control signals and video transmission. **Built with a modular, budget-friendly architecture** — start with a ~$15 base kit and upgrade as needed.

### 📦 Modular Tiers

| Tier | Name | Cost | Includes |
|------|------|------|----------|
| 🟢 **Base** | Starter | ~$15-20 | ESP32 + OLED + NRF24L01+ (2.4 GHz) + Web Dashboard + BLE Remote ID |
| 🟡 **Standard** | Hunter | ~$35-45 | Base + CC1101 (900 MHz) + RX5808 (5.8 GHz) + ML Classification |
| 🔴 **Pro** | Sentinel | ~$60-80 | Standard + GPS + SD Card Logger + LoRa Mesh Network |

**Optional add-ons**: 🎤 Acoustic Detection (~$5) | ⚔️ Countermeasures (auth required)

> 📖 **[Full Modular Guide →](docs/en/modules.md)**

### Features

- **Multi-band RF & Spectrum Scanning**: Hardware sweeping across 900 MHz, 2.4 GHz, and 5.8 GHz bands checking for analog and digital drone links.
- **Web Dashboard & Map**: Real-time dark-themed dashboard via WiFi with Leaflet.js interactive map, drone lists, and RSSI graphs.
- **Signal Fingerprinting**: Built-in `SignalDatabase` identifying known drone patterns (e.g., DJI OcuSync, FPV Analog, Crossfire) via band-matching and RSSI variance.
- **ESP-NOW Mesh**: Free, autonomous node-to-node network sharing threat alerts, heartbeats, and GPS telemetry across massive areas without extra hardware.
- **Power Management**: 4 dynamic power states (Full, Balanced, Low, Deep Sleep) with battery ADC monitoring and runtime estimates.
- **Auto-Calibration Tool**: Integrated baseline noise calibration directly from the Web-UI.
- **Alert System**: Non-blocking intelligent Buzzer and LED patterns scaling with Threat Levels (Info → Critical).
- **Remote ID**: FAA ANSI/CTA-2063 compliant BLE drone identification natively on the ESP32.
- **FreeRTOS Architecture**: Safe concurrent processing with hardware Watchdogs and SPI mutexes.

### Hardware Components (Standard Tier)

| Component | Model | Frequency | Purpose |
|-----------|-------|-----------|---------| 
| Microcontroller | ESP32 DevKit | - | Main processor + WiFi + BLE |
| RF Module 1 | CC1101 | 900 MHz | ISM band monitoring |
| RF Module 2 | NRF24L01+ | 2.4 GHz | WiFi/RC monitoring |
| RF Module 3 | RX5808 | 5.8 GHz | Video link monitoring |
| Display | OLED 128x64 (I2C) | - | Visual interface |
| Microphone (optional) | ICS-43434 MEMS | I2S | Acoustic detection |

### Pinout Configuration (v0.3.0 — Conflict-Free)

#### SPI Bus (Shared)
| Signal | ESP32 Pin |
|--------|-----------|
| MOSI   | GPIO 23   |
| MISO   | GPIO 19   |
| SCK    | GPIO 18   |

#### Chip Select Pins
| Module      | CS Pin    | CE Pin | Tier |
|-------------|-----------|--------|------|
| NRF24L01+   | GPIO 15   | GPIO 2 | Base+ |
| CC1101      | GPIO 5    | -      | Standard+ |
| RX5808      | GPIO 13   | -      | Standard+ |
| LoRa SX1276 | GPIO 26   | -      | Pro |
| SD Card     | GPIO 27   | -      | Pro |

#### I2C Bus (OLED Display)
| Signal | ESP32 Pin |
|--------|-----------|
| SDA    | GPIO 21   |
| SCL    | GPIO 22   |

#### Additional Connections
- **RX5808 RSSI**: GPIO 34 (ADC1_CH6)
- **Power**: 3.3V and GND to all modules

### Software Architecture

```
src/
├── main.cpp                    # Main application logic
├── countermeasures.h/cpp       # Active countermeasure system
├── drivers/
│   ├── cc1101.h/cpp           # CC1101 900MHz driver
│   ├── nrf24l01.h/cpp         # NRF24L01+ 2.4GHz driver
│   └── rx5808.h/cpp           # RX5808 5.8GHz driver
└── protocols/
    ├── mavlink_parser.h/cpp   # MAVLink protocol decoder
    └── crsf_parser.h/cpp      # CRSF/ExpressLRS decoder
```

### Build Instructions

```bash
# Clone repository
git clone https://github.com/bobberdolle1/SkySweep32.git
cd SkySweep32

# Build firmware
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

### Configuration

#### Enable Countermeasures (REQUIRES LEGAL AUTHORIZATION)

Edit `platformio.ini`:
```ini
build_flags = -DENABLE_COUNTERMEASURES
```

Edit `src/main.cpp`:
```cpp
counterMeasures.armSystem(true);  // Uncomment this line
```

### Legal Notice

⚠️ **WARNING**: Active RF countermeasures (jamming, protocol injection) are **ILLEGAL** in most jurisdictions without explicit authorization from regulatory authorities. Use of these features may result in:

- Criminal prosecution
- Heavy fines
- Equipment confiscation
- Interference with critical communications

**Authorized Use Cases**:
- Military and law enforcement operations
- Critical infrastructure protection (with permits)
- Conflict zones with appropriate authorization
- Research and development in controlled environments

**This project is for educational and authorized defense purposes only.**

### Documentation

- [Hardware Setup Guide](docs/en/hardware.md)
- [Software API Reference](docs/en/software.md)
- [Legal Compliance](docs/en/legal.md)

### License

GNU General Public License v3.0 - See [LICENSE](LICENSE) file for details.

---

<a name="russian"></a>
## 🇷🇺 Русский

### Обзор

SkySweep32 — продвинутая система пассивного обнаружения дронов на базе микроконтроллера ESP32. Система мониторит радиоэфир в трех диапазонах (900 МГц, 2.4 ГГц, 5.8 ГГц) для детекции сигналов управления БПЛА и видеопередачи. Включает опциональные возможности активного противодействия для авторизованных оборонных применений.

### Возможности

- **Мультидиапазонное и Спектральное сканирование**: Аппаратный скан эфира на 900 МГц, 2.4 ГГц и 5.8 ГГц диапазонах для детекции пультов, телеметрии и видеолинков.
- **Web-Дашборд и Интерактивная Карта**: Локальный веб-интерфейс по WiFi с картой (Leaflet.js) для трекинга дронов и операторов через Remote ID.
- **Сигнатурная База (Fingerprinting)**: Динамическое распознавание 8 типов дронов (DJI OcuSync, FPV, Crossfire и др.) через анализ дисперсии RSSI и паттернов "прыжков".
- **Своя Mesh-сеть (ESP-NOW)**: Самоорганизующаяся децентрализованная сеть оповещения между детекторами (0 рублей стоимости, использует WiFi чип ESP32).
- **Power Management (Батарея)**: Глубокий сон, скалер частоты ЦП и ADC-отслеживание батареи. Позволяет работать от 18650 днями. Автоматическая калибровка шума из UI.
- **Умная Система Уведомлений**: Неблокирующий диспетчер сигналов для зуммера (Buzzer) и LED с динамическими паттернами под каждый уровень угрозы.
- **Оценка угроз**: 5-уровневая классификация (НЕТ/НИЗКАЯ/СРЕДНЯЯ/ВЫСОКАЯ/КРИТИЧЕСКАЯ).
- **Активное противодействие (опционально, требуется легальная авторизация)**.

### Компоненты (Уровень Hunter)

| Компонент | Модель | Частота | Назначение |
|-----------|--------|---------|------------|
| Микроконтроллер | ESP32 DevKit | - | Основной процессор + WiFi/BLE |
| РЧ-модуль 1 | CC1101 | 900 МГц | Мониторинг ISM-диапазона |
| РЧ-модуль 2 | NRF24L01+ | 2.4 ГГц | Спектральное сканирование / RC |
| РЧ-модуль 3 | RX5808 | 5.8 ГГц | Мониторинг видеолинка |
| Дисплей | OLED 128x64 | - | Визуальный интерфейс |
| Индикация | Passive Buzzer | - | Алерты и ошибки |

### Распиновка (Бесконфликтная v0.4.0)

#### Шина SPI (общая)
| Сигнал | Пин ESP32 |
|--------|-----------|
| MOSI   | GPIO 23   |
| MISO   | GPIO 19   |
| SCK    | GPIO 18   |

#### Пины Chip Select (индивидуальные)
| Модуль      | CS пин    | CE пин (если есть) |
|-------------|-----------|--------------------|
| NRF24L01+   | GPIO 15   | GPIO 2             |
| CC1101      | GPIO 5    | -                  |
| RX5808      | GPIO 13   | -                  |
| LoRa SX1276 | GPIO 26   | -                  |
| SD Card     | GPIO 27   | -                  |

#### Шина I2C (OLED-дисплей)
| Сигнал | Пин ESP32 |
|--------|-----------|
| SDA    | GPIO 21   |
| SCL    | GPIO 22   |

#### Дополнительные подключения (Питание и Алерты)
- **Зуммер (Buzzer)**: GPIO 4
- **LED оповещения**: GPIO 2
- **ADC Батареи (100k/100k)**: GPIO 36
- **RX5808 RSSI**: GPIO 34 (ADC1_CH6)
- **Питание**: 3.3V и GND на все модули (регулятор LDO 1117 обязателен!)

### Архитектура ПО

```
src/
├── main.cpp                    # Основная логика приложения
├── countermeasures.h/cpp       # Система активного противодействия
├── drivers/
│   ├── cc1101.h/cpp           # Драйвер CC1101 900МГц
│   ├── nrf24l01.h/cpp         # Драйвер NRF24L01+ 2.4ГГц
│   └── rx5808.h/cpp           # Драйвер RX5808 5.8ГГц
└── protocols/
    ├── mavlink_parser.h/cpp   # Декодер протокола MAVLink
    └── crsf_parser.h/cpp      # Декодер CRSF/ExpressLRS
```

### Инструкции по сборке

```bash
# Клонировать репозиторий
git clone https://github.com/bobberdolle1/SkySweep32.git
cd SkySweep32

# Собрать прошивку
pio run

# Загрузить на ESP32
pio run --target upload

# Мониторинг Serial
pio device monitor
```

### Конфигурация

#### Включение противодействия (ТРЕБУЕТСЯ ЛЕГАЛЬНАЯ АВТОРИЗАЦИЯ)

Редактировать `platformio.ini`:
```ini
build_flags = -DENABLE_COUNTERMEASURES
```

Редактировать `src/main.cpp`:
```cpp
counterMeasures.armSystem(true);  // Раскомментировать эту строку
```

### Правовое уведомление

⚠️ **ВНИМАНИЕ**: Активные РЧ-противодействия (глушение, инъекция протоколов) **НЕЗАКОННЫ** в большинстве юрисдикций без явного разрешения регуляторных органов. Использование этих функций может привести к:

- Уголовному преследованию
- Крупным штрафам
- Конфискации оборудования
- Помехам критическим коммуникациям

**Разрешенные случаи использования**:
- Военные и правоохранительные операции
- Защита критической инфраструктуры (с разрешениями)
- Зоны боевых действий с соответствующей авторизацией
- Исследования и разработка в контролируемых условиях

**Этот проект предназначен только для образовательных и авторизованных оборонных целей.**

### Документация

- [Руководство по аппаратной части](docs/ru/hardware.md)
- [Справочник API ПО](docs/ru/software.md)
- [Правовое соответствие](docs/ru/legal.md)

### Лицензия

Стандартная общественная лицензия GNU v3.0 - См. файл [LICENSE](LICENSE) для деталей.

---

## Contributing | Вклад в проект

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details.

Приветствуются вклады! Пожалуйста, прочитайте [CONTRIBUTING.md](CONTRIBUTING.md) для деталей.

## Support | Поддержка

- GitHub Issues: [Report bugs](https://github.com/bobberdolle1/SkySweep32/issues)
- Discussions: [Community forum](https://github.com/bobberdolle1/SkySweep32/discussions)

---

**Developed with ❤️ for drone defense research | Разработано с ❤️ для исследований противодроновой защиты**
