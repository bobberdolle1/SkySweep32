#ifndef MESHTASTIC_CLIENT_H
#define MESHTASTIC_CLIENT_H

#include <Arduino.h>
#include <RadioLib.h>

#define LORA_CS_PIN 5
#define LORA_DIO0_PIN 2
#define LORA_DIO1_PIN 4
#define LORA_RESET_PIN 14
#define LORA_FREQUENCY 915.0
#define LORA_BANDWIDTH 125.0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODING_RATE 5
#define LORA_SYNC_WORD 0x12
#define LORA_TX_POWER 20

struct MeshPacket {
    uint32_t nodeID;
    uint8_t hopLimit;
    uint8_t hopCount;
    char payload[256];
    int16_t rssi;
    float snr;
    uint32_t timestamp;
};

struct DetectionAlert {
    double latitude;
    double longitude;
    float altitude;
    char droneID[32];
    int rssi;
    uint8_t threatLevel;
    uint32_t timestamp;
};

class MeshtasticClient {
private:
    SX1276* lora;
    bool isInitialized;
    uint32_t localNodeID;
    uint32_t lastTransmitTime;
    uint32_t transmitInterval;
    
    std::vector<MeshPacket> receivedPackets;
    std::vector<uint32_t> knownNodes;
    
    bool sendRawPacket(const uint8_t* data, size_t length);
    void processReceivedPacket(const uint8_t* data, size_t length, int16_t rssi, float snr);
    uint32_t generateNodeID();

public:
    MeshtasticClient();
    
    bool begin(float frequency = LORA_FREQUENCY);
    void update();
    
    bool broadcastDetectionAlert(const DetectionAlert& alert);
    bool sendDirectMessage(uint32_t targetNodeID, const char* message);
    
    uint8_t getReceivedPacketCount() const;
    MeshPacket getPacket(uint8_t index) const;
    void clearPackets();
    
    uint8_t getKnownNodeCount() const;
    uint32_t getNodeID(uint8_t index) const;
    
    void setTransmitInterval(uint32_t intervalMs);
    int16_t getLastRSSI() const;
    float getLastSNR() const;
};

#endif
