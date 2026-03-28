#ifndef NRF24L01_H
#define NRF24L01_H

#include <Arduino.h>
#include <SPI.h>

// NRF24L01+ Register Map
#define NRF24_CONFIG        0x00
#define NRF24_EN_AA         0x01
#define NRF24_EN_RXADDR     0x02
#define NRF24_SETUP_AW      0x03
#define NRF24_SETUP_RETR    0x04
#define NRF24_RF_CH         0x05
#define NRF24_RF_SETUP      0x06
#define NRF24_STATUS        0x07
#define NRF24_OBSERVE_TX    0x08
#define NRF24_RPD           0x09
#define NRF24_RX_ADDR_P0    0x0A
#define NRF24_RX_ADDR_P1    0x0B
#define NRF24_TX_ADDR       0x10
#define NRF24_RX_PW_P0      0x11
#define NRF24_FIFO_STATUS   0x17
#define NRF24_DYNPD         0x1C
#define NRF24_FEATURE       0x1D

// Commands
#define NRF24_R_REGISTER    0x00
#define NRF24_W_REGISTER    0x20
#define NRF24_R_RX_PAYLOAD  0x61
#define NRF24_W_TX_PAYLOAD  0xA0
#define NRF24_FLUSH_TX      0xE1
#define NRF24_FLUSH_RX      0xE2
#define NRF24_REUSE_TX_PL   0xE3
#define NRF24_NOP           0xFF

// CONFIG register bits
#define NRF24_MASK_RX_DR    0x40
#define NRF24_MASK_TX_DS    0x20
#define NRF24_MASK_MAX_RT   0x10
#define NRF24_EN_CRC        0x08
#define NRF24_CRCO          0x04
#define NRF24_PWR_UP        0x02
#define NRF24_PRIM_RX       0x01

// RF_SETUP register bits
#define NRF24_CONT_WAVE     0x80
#define NRF24_RF_DR_LOW     0x20
#define NRF24_RF_DR_HIGH    0x08
#define NRF24_RF_PWR_HIGH   0x06
#define NRF24_RF_PWR_LOW    0x00

class NRF24L01Driver {
private:
    uint8_t chipSelectPin;
    uint8_t chipEnablePin;
    SPIClass* spiInstance;
    uint8_t currentChannel;
    bool initialized;
    
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void writeRegisterMulti(uint8_t reg, uint8_t* data, uint8_t length);
    void readRegisterMulti(uint8_t reg, uint8_t* buffer, uint8_t length);
    uint8_t sendCommand(uint8_t command);
    
public:
    NRF24L01Driver(uint8_t csPin, uint8_t cePin, SPIClass* spi = &SPI);
    
    bool begin();
    void powerUp();
    void powerDown();
    void setRxMode();
    void setTxMode();
    
    void setChannel(uint8_t channel);
    void setDataRate(uint8_t rate); // 0=250kbps, 1=1Mbps, 2=2Mbps
    void setPowerLevel(uint8_t level); // 0-3
    void setAutoAck(bool enable);
    void setPayloadSize(uint8_t size);
    
    int8_t readRSSI(); // RPD-based RSSI estimation
    bool isCarrierDetected();
    uint8_t getRPD(); // Received Power Detector
    
    void transmitData(uint8_t* data, uint8_t length);
    uint8_t receiveData(uint8_t* buffer, uint8_t maxLength);
    
    void flushRx();
    void flushTx();
    void clearInterrupts();
    
    uint8_t getStatus();
    bool isConnected();
    void startListening();
    void stopListening();
};

#endif // NRF24L01_H
