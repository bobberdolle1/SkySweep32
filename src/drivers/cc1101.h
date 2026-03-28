#ifndef CC1101_H
#define CC1101_H

#include <Arduino.h>
#include <SPI.h>

// CC1101 Register Addresses
#define CC1101_IOCFG2       0x00
#define CC1101_IOCFG1       0x01
#define CC1101_IOCFG0       0x02
#define CC1101_FIFOTHR      0x03
#define CC1101_SYNC1        0x04
#define CC1101_SYNC0        0x05
#define CC1101_PKTLEN       0x06
#define CC1101_PKTCTRL1     0x07
#define CC1101_PKTCTRL0     0x08
#define CC1101_ADDR         0x09
#define CC1101_CHANNR       0x0A
#define CC1101_FSCTRL1      0x0B
#define CC1101_FSCTRL0      0x0C
#define CC1101_FREQ2        0x0D
#define CC1101_FREQ1        0x0E
#define CC1101_FREQ0        0x0F
#define CC1101_MDMCFG4      0x10
#define CC1101_MDMCFG3      0x11
#define CC1101_MDMCFG2      0x12
#define CC1101_MDMCFG1      0x13
#define CC1101_MDMCFG0      0x14
#define CC1101_DEVIATN      0x15
#define CC1101_MCSM2        0x16
#define CC1101_MCSM1        0x17
#define CC1101_MCSM0        0x18
#define CC1101_FOCCFG       0x19
#define CC1101_BSCFG        0x1A
#define CC1101_AGCCTRL2     0x1B
#define CC1101_AGCCTRL1     0x1C
#define CC1101_AGCCTRL0     0x1D
#define CC1101_FREND1       0x21
#define CC1101_FREND0       0x22
#define CC1101_FSCAL3       0x23
#define CC1101_FSCAL2       0x24
#define CC1101_FSCAL1       0x25
#define CC1101_FSCAL0       0x26
#define CC1101_TEST2        0x2C
#define CC1101_TEST1        0x2D
#define CC1101_TEST0        0x2E
#define CC1101_PATABLE      0x3E

// Status Registers
#define CC1101_PARTNUM      0x30
#define CC1101_VERSION      0x31
#define CC1101_FREQEST      0x32
#define CC1101_LQI          0x33
#define CC1101_RSSI         0x34
#define CC1101_MARCSTATE    0x35
#define CC1101_PKTSTATUS    0x38
#define CC1101_TXBYTES      0x3A
#define CC1101_RXBYTES      0x3B

// Command Strobes
#define CC1101_SRES         0x30
#define CC1101_SFSTXON      0x31
#define CC1101_SXOFF        0x32
#define CC1101_SCAL         0x33
#define CC1101_SRX          0x34
#define CC1101_STX          0x35
#define CC1101_SIDLE        0x36
#define CC1101_SWOR         0x38
#define CC1101_SPWD         0x39
#define CC1101_SFRX         0x3A
#define CC1101_SFTX         0x3B
#define CC1101_SWORRST      0x3C
#define CC1101_SNOP         0x3D

// Modulation Types
#define CC1101_MOD_2FSK     0x00
#define CC1101_MOD_GFSK     0x10
#define CC1101_MOD_ASK_OOK  0x30
#define CC1101_MOD_4FSK     0x40
#define CC1101_MOD_MSK      0x70

class CC1101Driver {
private:
    uint8_t chipSelectPin;
    SPIClass* spiInstance;
    uint32_t currentFrequency;
    bool initialized;
    
    void writeRegister(uint8_t address, uint8_t value);
    uint8_t readRegister(uint8_t address);
    void writeBurst(uint8_t address, uint8_t* data, uint8_t length);
    void readBurst(uint8_t address, uint8_t* buffer, uint8_t length);
    void sendStrobe(uint8_t strobe);
    uint8_t readStatusRegister(uint8_t address);
    
public:
    CC1101Driver(uint8_t csPin, SPIClass* spi = &SPI);
    
    bool begin();
    void reset();
    void setFrequency(uint32_t frequencyHz);
    void setModulation(uint8_t modulation);
    void setDataRate(uint32_t baud);
    void setChannel(uint8_t channel);
    void setPowerLevel(uint8_t power);
    
    void setRxMode();
    void setTxMode();
    void setIdleMode();
    
    int8_t readRSSI();
    uint8_t getLQI();
    bool isCarrierDetected();
    
    void transmitData(uint8_t* data, uint8_t length);
    uint8_t receiveData(uint8_t* buffer, uint8_t maxLength);
    
    void flushRxFIFO();
    void flushTxFIFO();
    
    uint8_t getChipVersion();
    bool isConnected();
};

#endif // CC1101_H
