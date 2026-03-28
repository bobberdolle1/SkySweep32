# Справочник API ПО

## Обзор архитектуры

SkySweep32 использует модульную архитектуру с уровнями аппаратной абстракции (HAL) для РЧ-модулей и парсерами протоколов для анализа сигналов.

```
Уровень приложения (main.cpp)
    ↓
Система противодействия (countermeasures.cpp)
    ↓
Парсеры протоколов (mavlink_parser.cpp, crsf_parser.cpp)
    ↓
Драйверы оборудования (cc1101.cpp, nrf24l01.cpp, rx5808.cpp)
    ↓
Аппаратная абстракция SPI/I2C
```

## API драйверов

### CC1101Driver (900 МГц)

```cpp
CC1101Driver cc1101(CS_PIN);

// Инициализация
bool begin();                           // Инициализировать модуль
void reset();                           // Аппаратный сброс
void setFrequency(uint32_t frequencyHz); // Установить центральную частоту
void setModulation(uint8_t modulation); // Установить тип модуляции
void setDataRate(uint32_t baud);        // Установить скорость передачи

// Режимы работы
void setRxMode();                       // Войти в режим приема
void setTxMode();                       // Войти в режим передачи
void setIdleMode();                     // Войти в режим ожидания

// Анализ сигнала
int8_t readRSSI();                      // Прочитать RSSI в дБм
uint8_t getLQI();                       // Индикатор качества связи
bool isCarrierDetected();               // Обнаружение несущей

// Передача данных
void transmitData(uint8_t* data, uint8_t length);
uint8_t receiveData(uint8_t* buffer, uint8_t maxLength);
```

**Пример использования**:
```cpp
cc1101.begin();
cc1101.setFrequency(915000000); // 915 МГц
cc1101.setRxMode();
int8_t rssi = cc1101.readRSSI();
Serial.printf("RSSI: %d дБм\n", rssi);
```

### NRF24L01Driver (2.4 ГГц)

```cpp
NRF24L01Driver nrf24(CS_PIN, CE_PIN);

// Инициализация
bool begin();
void powerUp();
void powerDown();

// Конфигурация
void setChannel(uint8_t channel);       // 0-125 (2400-2525 МГц)
void setDataRate(uint8_t rate);         // 0=250кбит/с, 1=1Мбит/с, 2=2Мбит/с
void setPowerLevel(uint8_t level);      // 0-3 (мин до макс)
void setPayloadSize(uint8_t size);      // 1-32 байта

// Работа
void startListening();                  // Войти в режим RX
void stopListening();                   // Выйти из режима RX
int8_t readRSSI();                      // Оценочный RSSI
uint8_t getRPD();                       // Детектор принятой мощности

// Передача данных
void transmitData(uint8_t* data, uint8_t length);
uint8_t receiveData(uint8_t* buffer, uint8_t maxLength);
```

### RX5808Driver (5.8 ГГц)

```cpp
RX5808Driver rx5808(CS_PIN, RSSI_PIN);

// Инициализация
bool begin();

// Выбор канала
void setChannel(uint8_t band, uint8_t channel);
void setFrequency(uint16_t frequencyMHz);

// Измерение сигнала
int readRSSI();                         // 0-100 процентов
int readRSSIRaw();                      // Сырое значение АЦП

// Сканирование
void scanBand(uint8_t band, int* rssiValues);
RX5808Channel findStrongestChannel();
```

**Частотные диапазоны**:
- `RX5808_BAND_A`: Boscam A (5725-5865 МГц)
- `RX5808_BAND_B`: Boscam B (5733-5866 МГц)
- `RX5808_BAND_E`: Boscam E/DJI (5645-5945 МГц)
- `RX5808_BAND_F`: Fatshark (5740-5880 МГц)
- `RX5808_BAND_R`: Raceband (5658-5917 МГц)

## API парсеров протоколов

### MAVLinkParser

```cpp
MAVLinkParser mavlink;

// Парсинг
bool parseByte(uint8_t byte);
bool parseBuffer(uint8_t* data, uint16_t length);
MAVLinkPacket getPacket();

// Построители пакетов (для инъекции)
void buildRTHCommand(uint8_t* buffer, uint8_t* length);
void buildLandCommand(uint8_t* buffer, uint8_t* length);
void buildDisarmCommand(uint8_t* buffer, uint8_t* length);

// Анализ
bool isHeartbeat(MAVLinkPacket* packet);
bool isGPSData(MAVLinkPacket* packet);
MAVLinkHeartbeat parseHeartbeat(MAVLinkPacket* packet);
MAVLinkGPS parseGPS(MAVLinkPacket* packet);
```

### CRSFParser

```cpp
CRSFParser crsf;

// Парсинг
bool parseByte(uint8_t byte);
bool parseBuffer(uint8_t* data, uint16_t length);
CRSFPacket getPacket();

// Анализ
bool isLinkStats(CRSFPacket* packet);
CRSFLinkStats parseLinkStats(CRSFPacket* packet);
int8_t getRSSIFromLinkStats(CRSFLinkStats* stats);
```

## API системы противодействия

```cpp
CountermeasureSystem cm;

// Управление системой
void initialize();
void armSystem(bool armed);
bool isArmed();

// Оценка угроз
ThreatLevel assessThreat(uint8_t moduleIndex, int rssiValue);
ThreatData getCurrentThreat();

// Ручные противодействия
bool executeCountermeasure(CountermeasureType type, uint8_t chipSelectPin);

// Автоматический ответ
void autoRespond(uint8_t moduleIndex, int rssiValue, uint8_t chipSelectPin);
```

**Уровни угроз**:
- `THREAT_NONE`: Угроза не обнаружена
- `THREAT_LOW`: RSSI 45-59 дБм
- `THREAT_MEDIUM`: RSSI 60-74 дБм
- `THREAT_HIGH`: RSSI 75-84 дБм
- `THREAT_CRITICAL`: RSSI ≥85 дБм

**Типы противодействий**:
- `CM_JAMMING_BROADBAND`: Широкополосная инъекция шума
- `CM_JAMMING_TARGETED`: Глушение по протоколу
- `CM_PROTOCOL_INJECTION`: Инъекция команд (RTH/посадка)
- `CM_DEAUTH_FLOOD`: WiFi деаутентификация

**Пример использования**:
```cpp
cm.initialize();
cm.armSystem(true);

ThreatLevel threat = cm.assessThreat(0, rssiValue);
if (threat >= THREAT_HIGH) {
    cm.executeCountermeasure(CM_PROTOCOL_INJECTION, CS_CC1101);
}
```

## Макросы конфигурации

### Флаги времени компиляции

```cpp
// Включить активные противодействия (platformio.ini)
#define ENABLE_COUNTERMEASURES

// Пороги RSSI (countermeasures.cpp)
#define RSSI_THRESHOLD_LOW      45
#define RSSI_THRESHOLD_MEDIUM   60
#define RSSI_THRESHOLD_HIGH     75
#define RSSI_THRESHOLD_CRITICAL 85
```

### Определения пинов

```cpp
// Пины SPI (main.cpp)
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK  18

// Пины Chip Select
#define CS_CC1101    5
#define CS_NRF24L01  17
#define CS_RX5808    16

// Пины I2C
#define I2C_SDA 21
#define I2C_SCL 22

// Дополнительные пины
#define RX5808_RSSI_PIN 34
#define NRF24_CE_PIN    4
```

## Метрики производительности

- **Частота сканирования**: ~3 Гц (все три диапазона)
- **Частота обновления RSSI**: ~10 Гц на модуль
- **Задержка обнаружения протокола**: <100мс
- **Время отклика противодействия**: <500мс

## Использование памяти

- **Flash**: ~450 КБ (с включенными противодействиями)
- **RAM**: ~45 КБ
- **Stack**: ~8 КБ на задачу

## Следующие шаги

- Просмотрите [Правовое соответствие](legal.md) перед включением противодействий
- См. [Руководство по аппаратной части](hardware.md) для проверки проводки
