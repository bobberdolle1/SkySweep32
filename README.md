# SkySweep32 | Пассивный детектор БПЛА

**Multi-band passive drone detector | Мультидиапазонный пассивный детектор дронов**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Build: PlatformIO](https://img.shields.io/badge/Build-PlatformIO-orange.svg)](https://platformio.org/)

[English](#english) | [Русский](#russian)

---

<a name="english"></a>
## 🇬🇧 English

### Overview

SkySweep32 is an advanced passive drone detection system based on the ESP32 microcontroller. It monitors radio spectrum across three frequency bands (900 MHz, 2.4 GHz, 5.8 GHz) to detect UAV control signals and video transmission. The system includes optional active countermeasure capabilities for authorized defense applications.

### Features

- **Multi-band RF Scanning**: Simultaneous monitoring of 900 MHz, 2.4 GHz, and 5.8 GHz bands
- **Protocol Detection**: Identifies MAVLink, CRSF/ExpressLRS, DJI, and analog video signals
- **Real-time OLED Display**: Visual feedback with signal strength bars and threat levels
- **Threat Assessment**: 5-level classification (NONE/LOW/MEDIUM/HIGH/CRITICAL)
- **Active Countermeasures** (optional, requires legal authorization):
  - Wideband jamming
  - Protocol-specific jamming
  - Command injection (RTH/Land)
  - Deauthentication attacks

### Hardware Components

| Component | Model | Frequency | Purpose |
|-----------|-------|-----------|---------|
| Microcontroller | ESP32 DevKit | - | Main processor |
| RF Module 1 | CC1101 | 900 MHz | ISM band monitoring |
| RF Module 2 | NRF24L01+ | 2.4 GHz | WiFi/RC monitoring |
| RF Module 3 | RX5808 | 5.8 GHz | Video link monitoring |
| Display | OLED 128x64 (I2C) | - | Visual interface |

### Pinout Configuration

#### SPI Bus (Shared)
| Signal | ESP32 Pin |
|--------|-----------|
| MOSI   | GPIO 23   |
| MISO   | GPIO 19   |
| SCK    | GPIO 18   |

#### Chip Select Pins (Individual)
| Module      | CS Pin    | CE Pin (if applicable) |
|-------------|-----------|------------------------|
| CC1101      | GPIO 5    | -                      |
| NRF24L01+   | GPIO 17   | GPIO 4                 |
| RX5808      | GPIO 16   | -                      |

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

MIT License - See [LICENSE](LICENSE) file for details.

---

<a name="russian"></a>
## 🇷🇺 Русский

### Обзор

SkySweep32 — продвинутая система пассивного обнаружения дронов на базе микроконтроллера ESP32. Система мониторит радиоэфир в трех диапазонах (900 МГц, 2.4 ГГц, 5.8 ГГц) для детекции сигналов управления БПЛА и видеопередачи. Включает опциональные возможности активного противодействия для авторизованных оборонных применений.

### Возможности

- **Мультидиапазонное сканирование**: Одновременный мониторинг 900 МГц, 2.4 ГГц и 5.8 ГГц
- **Распознавание протоколов**: Идентификация MAVLink, CRSF/ExpressLRS, DJI и аналогового видео
- **OLED-дисплей в реальном времени**: Визуальная индикация с графиками уровня сигнала
- **Оценка угроз**: 5-уровневая классификация (НЕТ/НИЗКАЯ/СРЕДНЯЯ/ВЫСОКАЯ/КРИТИЧЕСКАЯ)
- **Активное противодействие** (опционально, требуется легальная авторизация):
  - Широкополосное глушение
  - Целевое глушение по протоколу
  - Инъекция команд (RTH/посадка)
  - Deauth-атаки

### Компоненты

| Компонент | Модель | Частота | Назначение |
|-----------|--------|---------|------------|
| Микроконтроллер | ESP32 DevKit | - | Основной процессор |
| РЧ-модуль 1 | CC1101 | 900 МГц | Мониторинг ISM-диапазона |
| РЧ-модуль 2 | NRF24L01+ | 2.4 ГГц | Мониторинг WiFi/RC |
| РЧ-модуль 3 | RX5808 | 5.8 ГГц | Мониторинг видеолинка |
| Дисплей | OLED 128x64 (I2C) | - | Визуальный интерфейс |

### Распиновка

#### Шина SPI (общая)
| Сигнал | Пин ESP32 |
|--------|-----------|
| MOSI   | GPIO 23   |
| MISO   | GPIO 19   |
| SCK    | GPIO 18   |

#### Пины Chip Select (индивидуальные)
| Модуль      | CS пин    | CE пин (если есть) |
|-------------|-----------|---------------------|
| CC1101      | GPIO 5    | -                   |
| NRF24L01+   | GPIO 17   | GPIO 4              |
| RX5808      | GPIO 16   | -                   |

#### Шина I2C (OLED-дисплей)
| Сигнал | Пин ESP32 |
|--------|-----------|
| SDA    | GPIO 21   |
| SCL    | GPIO 22   |

#### Дополнительные подключения
- **RX5808 RSSI**: GPIO 34 (ADC1_CH6)
- **Питание**: 3.3V и GND на все модули

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

Лицензия MIT - См. файл [LICENSE](LICENSE) для деталей.

---

## Contributing | Вклад в проект

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details.

Приветствуются вклады! Пожалуйста, прочитайте [CONTRIBUTING.md](CONTRIBUTING.md) для деталей.

## Support | Поддержка

- GitHub Issues: [Report bugs](https://github.com/bobberdolle1/SkySweep32/issues)
- Discussions: [Community forum](https://github.com/bobberdolle1/SkySweep32/discussions)

---

**Developed with ❤️ for drone defense research | Разработано с ❤️ для исследований противодроновой защиты**
