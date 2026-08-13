#include "spi_manager.h"

SPIManager spiManager;

SPIManager::SPIManager() : spi(&SPI), initialized(false), mutex(nullptr) {}

bool SPIManager::begin() {
    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) {
        Serial.println("[SPI] Failed to create mutex");
        return false;
    }

    spi->begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
    spi->setFrequency(1000000);
    for (const uint8_t chipSelect : {PIN_CC1101_CS, PIN_SX1281_CS, PIN_SD_CS}) {
        pinMode(chipSelect, OUTPUT);
        digitalWrite(chipSelect, HIGH);
    }

    initialized = true;
    Serial.printf("[SPI] MOSI:%d MISO:%d SCK:%d\n", PIN_SPI_MOSI, PIN_SPI_MISO,
                  PIN_SPI_SCK);
    return true;
}

bool SPIManager::acquire(TickType_t timeout) {
    return mutex != nullptr && xSemaphoreTake(mutex, timeout) == pdTRUE;
}

void SPIManager::release() {
    if (mutex != nullptr) xSemaphoreGive(mutex);
}

void SPIManager::selectDevice(uint8_t chipSelect) {
    deselectAll();
    digitalWrite(chipSelect, LOW);
    delayMicroseconds(10);
}

void SPIManager::deselectAll() {
    digitalWrite(PIN_CC1101_CS, HIGH);
    digitalWrite(PIN_SX1281_CS, HIGH);
    digitalWrite(PIN_SD_CS, HIGH);
}

uint8_t SPIManager::transfer(uint8_t chipSelect, uint8_t data) {
    selectDevice(chipSelect);
    const uint8_t result = spi->transfer(data);
    deselectAll();
    return result;
}

void SPIManager::transferBulk(uint8_t chipSelect, uint8_t* data, size_t length) {
    selectDevice(chipSelect);
    spi->transfer(data, length);
    deselectAll();
}
