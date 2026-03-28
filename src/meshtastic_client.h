#ifndef MESHTASTIC_CLIENT_H
#define MESHTASTIC_CLIENT_H

#include <Arduino.h>
#include "config.h"

#ifdef MODULE_LORA

#include <RadioLib.h>

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

#endif // MODULE_LORA
#endif // MESHTASTIC_CLIENT_H
