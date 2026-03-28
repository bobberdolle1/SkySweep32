#include "spi_manager.h"

// Global instance
SPIManager spiManager;

SPIManager::SPIManager() : spi(&SPI), initialized(false), mutex(nullptr) {
}

bool SPIManager::begin() {
    // Create mutex
    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) {
        Serial.println("[SPI] Failed to create mutex");
        return false;
    }
    
    // Initialize SPI bus
    spi->begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
    spi->setFrequency(1000000); // 1 MHz safe default
    
    // Configure all CS pins as OUTPUT, deselected (HIGH)
    #ifdef MODULE_CC1101
    pinMode(PIN_CC1101_CS, OUTPUT);
    digitalWrite(PIN_CC1101_CS, HIGH);
    #endif
    
    #ifdef MODULE_NRF24
    pinMode(PIN_NRF24_CS, OUTPUT);
    digitalWrite(PIN_NRF24_CS, HIGH);
    #endif
    
    #ifdef MODULE_RX5808
    pinMode(PIN_RX5808_CS, OUTPUT);
    digitalWrite(PIN_RX5808_CS, HIGH);
    #endif
    
    #ifdef MODULE_LORA
    pinMode(PIN_LORA_CS, OUTPUT);
    digitalWrite(PIN_LORA_CS, HIGH);
    #endif
    
    #ifdef MODULE_SD_CARD
    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);
    #endif
    
    initialized = true;
    Serial.printf("[SPI] Initialized - MOSI:%d MISO:%d SCK:%d\n", 
                  PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK);
    return true;
}

bool SPIManager::acquire(TickType_t timeout) {
    if (mutex == nullptr) return false;
    return xSemaphoreTake(mutex, timeout) == pdTRUE;
}

void SPIManager::release() {
    if (mutex != nullptr) {
        xSemaphoreGive(mutex);
    }
}

void SPIManager::selectDevice(uint8_t csPin) {
    deselectAll();
    digitalWrite(csPin, LOW);
    delayMicroseconds(10);
}

void SPIManager::deselectAll() {
    #ifdef MODULE_CC1101
    digitalWrite(PIN_CC1101_CS, HIGH);
    #endif
    #ifdef MODULE_NRF24
    digitalWrite(PIN_NRF24_CS, HIGH);
    #endif
    #ifdef MODULE_RX5808
    digitalWrite(PIN_RX5808_CS, HIGH);
    #endif
    #ifdef MODULE_LORA
    digitalWrite(PIN_LORA_CS, HIGH);
    #endif
    #ifdef MODULE_SD_CARD
    digitalWrite(PIN_SD_CS, HIGH);
    #endif
}

uint8_t SPIManager::transfer(uint8_t csPin, uint8_t data) {
    selectDevice(csPin);
    uint8_t result = spi->transfer(data);
    deselectAll();
    return result;
}

void SPIManager::transferBulk(uint8_t csPin, uint8_t* data, size_t length) {
    selectDevice(csPin);
    spi->transfer(data, length);
    deselectAll();
}
