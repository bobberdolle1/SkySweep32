#include "espnow_mesh.h"

ESPNowMesh espNowMesh;
ESPNowMesh* ESPNowMesh::instance = nullptr;

ESPNowMesh::ESPNowMesh()
    : myNodeID(0), sequenceCounter(0), peerCount(0),
      initialized(false), lastHeartbeat(0), heartbeatIntervalMs(15000),
      msgSent(0), msgReceived(0), msgFailed(0) {
    memset(peers, 0, sizeof(peers));
}

// --- Static callbacks (ESP-NOW requires C-style function pointers) ---

void ESPNowMesh::onReceiveCB(const uint8_t* mac, const uint8_t* data, int len) {
    if (instance && len >= sizeof(uint8_t) * 4) {  // Minimum header size
        instance->handleMessage(mac, (const ESPNowMessage*)data);
    }
}

void ESPNowMesh::onSendCB(const uint8_t* mac, esp_now_send_status_t status) {
    if (instance) {
        if (status == ESP_NOW_SEND_SUCCESS) {
            instance->msgSent++;
        } else {
            instance->msgFailed++;
        }
    }
}

// --- Init ---

bool ESPNowMesh::begin(uint8_t nodeID) {
    instance = this;
    
    // Auto-generate node ID from last byte of MAC if not specified
    if (nodeID == 0) {
        uint8_t mac[6];
        WiFi.macAddress(mac);
        myNodeID = mac[5];
    } else {
        myNodeID = nodeID;
    }
    
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("[MESH] ESP-NOW init failed");
        return false;
    }
    
    // Register callbacks
    esp_now_register_recv_cb(onReceiveCB);
    esp_now_register_send_cb(onSendCB);
    
    // Add broadcast peer
    esp_now_peer_info_t broadcastPeer;
    memset(&broadcastPeer, 0, sizeof(broadcastPeer));
    uint8_t broadcastAddr[] = ESPNOW_BROADCAST_ADDR;
    memcpy(broadcastPeer.peer_addr, broadcastAddr, 6);
    broadcastPeer.channel = 0;  // Current channel
    broadcastPeer.encrypt = false;
    
    if (esp_now_add_peer(&broadcastPeer) != ESP_OK) {
        Serial.println("[MESH] Failed to add broadcast peer");
    }
    
    initialized = true;
    Serial.printf("[MESH] ESP-NOW ready, Node ID: %d (0x%02X)\n", myNodeID, myNodeID);
    
    // Send initial heartbeat
    sendHeartbeat();
    
    return true;
}

void ESPNowMesh::stop() {
    if (initialized) {
        esp_now_deinit();
        initialized = false;
        instance = nullptr;
    }
}

void ESPNowMesh::update() {
    if (!initialized) return;
    
    // Send periodic heartbeat
    if (millis() - lastHeartbeat >= heartbeatIntervalMs) {
        sendHeartbeat();
        lastHeartbeat = millis();
    }
    
    // Cleanup stale peers (not heard from in 60 seconds)
    removeStale(60000);
}

// --- Message handling ---

void ESPNowMesh::handleMessage(const uint8_t* mac, const ESPNowMessage* msg) {
    msgReceived++;
    
    // Ignore own messages (loopback)
    if (msg->nodeID == myNodeID) return;
    
    // Find or add peer
    int peerIdx = findPeer(mac);
    if (peerIdx < 0) {
        peerIdx = addPeer(mac, msg->nodeID);
        if (peerIdx < 0) return;  // Peer table full
    }
    
    peers[peerIdx].lastSeen = millis();
    peers[peerIdx].active = true;
    
    switch (msg->type) {
        case MSG_HEARTBEAT:
            peers[peerIdx].batteryV = msg->payload.heartbeat.batteryV;
            peers[peerIdx].threatLevel = msg->payload.heartbeat.threatLevel;
            Serial.printf("[MESH] Heartbeat from Node %d (bat: %.2fV, threat: %d)\n",
                          msg->nodeID, msg->payload.heartbeat.batteryV,
                          msg->payload.heartbeat.threatLevel);
            break;
            
        case MSG_THREAT_ALERT:
            Serial.printf("[MESH] ⚠️ THREAT from Node %d: RSSI=%d band=%d proto=%d (%.4f, %.4f)\n",
                          msg->nodeID,
                          msg->payload.threat.rssi,
                          msg->payload.threat.band,
                          msg->payload.threat.protocol,
                          msg->payload.threat.lat,
                          msg->payload.threat.lon);
            break;
            
        case MSG_GPS_POS:
            Serial.printf("[MESH] Node %d GPS: %.6f, %.6f alt=%.0fm sats=%d\n",
                          msg->nodeID,
                          msg->payload.gps.lat, msg->payload.gps.lon,
                          msg->payload.gps.alt, msg->payload.gps.satellites);
            break;
            
        default:
            Serial.printf("[MESH] Unknown msg type 0x%02X from Node %d\n", msg->type, msg->nodeID);
            break;
    }
}

// --- Peer management ---

int ESPNowMesh::findPeer(const uint8_t* mac) {
    for (uint8_t i = 0; i < peerCount; i++) {
        if (memcmp(peers[i].macAddr, mac, 6) == 0) {
            return i;
        }
    }
    return -1;
}

int ESPNowMesh::addPeer(const uint8_t* mac, uint8_t nodeID) {
    if (peerCount >= ESPNOW_MAX_PEERS) {
        Serial.println("[MESH] Peer table full");
        return -1;
    }
    
    // Register with ESP-NOW
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_add_peer(&peerInfo);
    }
    
    // Add to our table
    memcpy(peers[peerCount].macAddr, mac, 6);
    peers[peerCount].nodeID = nodeID;
    peers[peerCount].lastSeen = millis();
    peers[peerCount].active = true;
    
    Serial.printf("[MESH] New peer: Node %d (%02X:%02X:%02X:%02X:%02X:%02X)\n",
                  nodeID, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return peerCount++;
}

void ESPNowMesh::removeStale(uint32_t timeoutMs) {
    for (uint8_t i = 0; i < peerCount; i++) {
        if (peers[i].active && (millis() - peers[i].lastSeen > timeoutMs)) {
            peers[i].active = false;
            Serial.printf("[MESH] Node %d timed out\n", peers[i].nodeID);
        }
    }
}

// --- Send functions ---

bool ESPNowMesh::broadcast(const ESPNowMessage& msg) {
    if (!initialized) return false;
    
    uint8_t broadcastAddr[] = ESPNOW_BROADCAST_ADDR;
    esp_err_t result = esp_now_send(broadcastAddr, (const uint8_t*)&msg, sizeof(ESPNowMessage));
    return (result == ESP_OK);
}

bool ESPNowMesh::sendTo(const uint8_t* mac, const ESPNowMessage& msg) {
    if (!initialized) return false;
    
    esp_err_t result = esp_now_send(mac, (const uint8_t*)&msg, sizeof(ESPNowMessage));
    return (result == ESP_OK);
}

bool ESPNowMesh::sendHeartbeat() {
    ESPNowMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_HEARTBEAT;
    msg.nodeID = myNodeID;
    msg.sequence = sequenceCounter++;
    msg.timestamp = millis();
    
    // Include power manager data if available
    extern class PowerManager powerManager;
    msg.payload.heartbeat.batteryV = powerManager.getBatteryVoltage();
    msg.payload.heartbeat.threatLevel = 0;  // Will be set by main loop
    msg.payload.heartbeat.activeModules = RF_MODULE_COUNT;
    
    return broadcast(msg);
}

bool ESPNowMesh::sendThreatAlert(int8_t rssi, uint8_t band, uint8_t protocol, float lat, float lon) {
    ESPNowMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_THREAT_ALERT;
    msg.nodeID = myNodeID;
    msg.sequence = sequenceCounter++;
    msg.timestamp = millis();
    msg.payload.threat.rssi = rssi;
    msg.payload.threat.band = band;
    msg.payload.threat.protocol = protocol;
    msg.payload.threat.lat = lat;
    msg.payload.threat.lon = lon;
    
    Serial.printf("[MESH] Broadcasting threat alert: RSSI=%d band=%d\n", rssi, band);
    return broadcast(msg);
}

bool ESPNowMesh::sendGPSPosition(double lat, double lon, float alt, uint8_t sats) {
    ESPNowMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_GPS_POS;
    msg.nodeID = myNodeID;
    msg.sequence = sequenceCounter++;
    msg.timestamp = millis();
    msg.payload.gps.lat = lat;
    msg.payload.gps.lon = lon;
    msg.payload.gps.alt = alt;
    msg.payload.gps.satellites = sats;
    
    return broadcast(msg);
}

uint8_t ESPNowMesh::getActivePeerCount() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < peerCount; i++) {
        if (peers[i].active) count++;
    }
    return count;
}
