#ifndef SPI_MANAGER_H
#define SPI_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include "config.h"

// FreeRTOS mutex for SPI bus sharing
class SPIManager {
private:
    SemaphoreHandle_t mutex;
    SPIClass* spi;
    bool initialized;

public:
    SPIManager();
    
    /// Initialize SPI bus and mutex
    bool begin();
    
    /// Acquire SPI bus (blocking with timeout)
    /// @param timeout Max wait time in ticks (portMAX_DELAY = infinite)
    /// @return true if acquired, false if timeout
    bool acquire(TickType_t timeout = portMAX_DELAY);
    
    /// Release SPI bus
    void release();
    
    /// Select SPI device (deselects all others first)
    void selectDevice(uint8_t csPin);
    
    /// Deselect all SPI devices
    void deselectAll();
    
    /// Get SPI instance (use between acquire/release)
    SPIClass* getSPI() { return spi; }
    
    /// Transfer with device selection (convenience method)
    uint8_t transfer(uint8_t csPin, uint8_t data);
    void transferBulk(uint8_t csPin, uint8_t* data, size_t length);
};

// Global SPI manager instance
extern SPIManager spiManager;

#endif // SPI_MANAGER_H
