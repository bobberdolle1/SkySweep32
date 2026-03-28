#ifndef ESPNOW_MESH_H
#define ESPNOW_MESH_H

#include <Arduino.h>
#include "config.h"

// ESP-NOW mesh works alongside AP mode — no extra hardware needed
#include <esp_now.h>
#include <WiFi.h>

#define ESPNOW_MAX_PEERS       10
#define ESPNOW_MSG_MAX_LEN     200
#define ESPNOW_BROADCAST_ADDR  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

// Message types
enum ESPNowMsgType {
    MSG_HEARTBEAT    = 0x01,   // Node alive ping
    MSG_THREAT_ALERT = 0x02,   // Drone detection alert
    MSG_RF_DATA      = 0x03,   // Raw RF scan data share
    MSG_GPS_POS      = 0x04,   // Node GPS position
    MSG_SYNC         = 0x05,   // Time sync
    MSG_COMMAND      = 0x06    // Remote command
};

// Packed message structure (fits in 250-byte ESP-NOW limit)
struct __attribute__((packed)) ESPNowMessage {
    uint8_t  type;              // ESPNowMsgType
    uint8_t  nodeID;            // Sender node ID (0-255)
    uint16_t sequence;          // Message sequence number
    uint32_t timestamp;         // millis() at send time
    union {
        struct {
            float   batteryV; 
            uint8_t threatLevel;
            uint8_t activeModules;
        } heartbeat;
        struct {
            int8_t  rssi;
            uint8_t band;       // 0=433, 1=868, 2=915, 3=2400, 4=5800
            uint8_t protocol;   // DroneProtocol enum
            float   lat;
            float   lon;
        } threat;
        struct {
            double  lat;
            double  lon;
            float   alt;
            uint8_t satellites;
        } gps;
        uint8_t raw[200];       // Generic payload
    } payload;
};

// Peer node info
struct PeerNode {
    uint8_t macAddr[6];
    uint8_t nodeID;
    uint32_t lastSeen;
    float   batteryV;
    uint8_t threatLevel;
    bool    active;
};

class ESPNowMesh {
private:
    uint8_t myNodeID;
    uint16_t sequenceCounter;
    PeerNode peers[ESPNOW_MAX_PEERS];
    uint8_t peerCount;
    bool initialized;
    uint32_t lastHeartbeat;
    uint32_t heartbeatIntervalMs;
    
    // Stats
    uint32_t msgSent;
    uint32_t msgReceived;
    uint32_t msgFailed;
    
    static ESPNowMesh* instance;  // For callback
    static void onReceiveCB(const uint8_t* mac, const uint8_t* data, int len);
    static void onSendCB(const uint8_t* mac, esp_now_send_status_t status);
    
    void handleMessage(const uint8_t* mac, const ESPNowMessage* msg);
    int findPeer(const uint8_t* mac);
    int addPeer(const uint8_t* mac, uint8_t nodeID);
    void removeStale(uint32_t timeoutMs = 60000);

public:
    ESPNowMesh();
    
    bool begin(uint8_t nodeID = 0);  // 0 = auto-generate from MAC
    void update();
    void stop();
    
    // Send functions
    bool broadcast(const ESPNowMessage& msg);
    bool sendTo(const uint8_t* mac, const ESPNowMessage& msg);
    
    // High-level sends
    bool sendHeartbeat();
    bool sendThreatAlert(int8_t rssi, uint8_t band, uint8_t protocol, float lat, float lon);
    bool sendGPSPosition(double lat, double lon, float alt, uint8_t sats);
    
    // Getters
    uint8_t getNodeID() const { return myNodeID; }
    uint8_t getPeerCount() const { return peerCount; }
    uint8_t getActivePeerCount() const;
    const PeerNode* getPeers() const { return peers; }
    
    // Stats
    uint32_t getSentCount() const { return msgSent; }
    uint32_t getReceivedCount() const { return msgReceived; }
    
    void setHeartbeatInterval(uint32_t ms) { heartbeatIntervalMs = ms; }
};

extern ESPNowMesh espNowMesh;

#endif // ESPNOW_MESH_H
