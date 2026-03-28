# Руководство по настройке ПО

## Быстрый старт

### 1. Установка PlatformIO

Скачайте [PlatformIO IDE](https://platformio.org/install/ide) (расширение VS Code).

### 2. Клонирование

```bash
git clone https://github.com/bobberdolle1/SkySweep32.git
cd SkySweep32
```

### 3. Выбор уровня

В `platformio.ini` установите `build_flags`:

```ini
# Base (~$15): ESP32 + OLED + NRF24L01+
build_flags = -DTIER_BASE

# Standard (~$35): + CC1101 + RX5808 (ПО УМОЛЧАНИЮ)
build_flags = -DTIER_STANDARD

# Pro (~$60+): + GPS + SD Card + LoRa
build_flags = -DTIER_PRO
```

### 4. Сборка и загрузка

```bash
pio run -t upload
```

### 5. Доступ к дашборду

1. Подключитесь к WiFi **SkySweep32** (пароль: `skysweep32`)
2. Откройте `http://192.168.4.1`

---

## Runtime-конфигурация

Настройки хранятся в JSON на SPIFFS (внутренняя флеш). Изменения без перекомпиляции.

### Через REST API

```bash
# Текущая конфигурация
curl http://192.168.4.1/api/config

# Обновить настройки (частичное обновление)
curl -X POST http://192.168.4.1/api/config \
  -H "Content-Type: application/json" \
  -d '{"thresholds":{"low":50},"rfScanMs":200}'

# Сброс к дефолтам
curl -X POST http://192.168.4.1/api/config/reset
```

### Поля конфигурации

| Поле | По умолч. | Описание |
|------|-----------|----------|
| `wifi.ssid` | "SkySweep32" | Имя WiFi сети |
| `wifi.password` | "skysweep32" | Пароль WiFi |
| `wifi.apMode` | true | Режим AP (true) или STA (false) |
| `thresholds.low` | 45 | RSSI порог LOW |
| `thresholds.medium` | 60 | RSSI порог MEDIUM |
| `thresholds.high` | 75 | RSSI порог HIGH |
| `thresholds.critical` | 85 | RSSI порог CRITICAL |
| `rfScanMs` | 100 | Интервал опроса РЧ (мс) |
| `lora.freq` | 915.0 | Частота LoRa (МГц) |
| `logLevel` | 1 | 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR |

---

## OTA-обновления

```bash
# Сборка
pio run

# Загрузка OTA
curl -X POST http://192.168.4.1/api/ota \
  -F "firmware=@.pio/build/esp32dev/firmware.bin"
```

---

## API Reference

| Эндпоинт | Метод | Описание |
|----------|-------|----------|
| `/` | GET | Веб-дашборд |
| `/api/status` | GET | Статус системы |
| `/api/config` | GET | Текущая конфигурация |
| `/api/config` | POST | Обновить конфигурацию |
| `/api/config/reset` | POST | Сброс конфигурации |
| `/api/ota` | POST | Загрузка прошивки |
| `/ws` | WebSocket | Поток данных |

---

## Флаги сборки

| Флаг | Описание |
|------|----------|
| `-DTIER_BASE` | Базовый уровень |
| `-DTIER_STANDARD` | Стандартный уровень |
| `-DTIER_PRO` | Профессиональный уровень |
| `-DMODULE_ACOUSTIC` | Акустическое обнаружение |
| `-DENABLE_COUNTERMEASURES` | Активные контрмеры ⚠️ |
