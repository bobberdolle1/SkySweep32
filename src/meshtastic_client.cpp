#include "meshtastic_client.h"

MeshtasticClient::MeshtasticClient() 
    : isInitialized(false), lastTransmitTime(0), transmitInterval(30000), lora(nullptr) {
    localNodeID = generateNodeID();
}

bool MeshtasticClient::begin(float frequency) {
    Serial.println("[Meshtastic] Initializing LoRa module...");
    
    lora = new SX1276(new Module(LORA_CS_PIN, LORA_DIO0_PIN, LORA_RESET_PIN, LORA_DIO1_PIN));
    
    int state = lora->begin(frequency, LORA_BANDWIDTH, LORA_SPREADING_FACTOR, 
                            LORA_CODING_RATE, LORA_SYNC_WORD, LORA_TX_POWER);
    
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[Meshtastic] LoRa init failed, code: %d\n", state);
        isInitialized = false;
        return false;
    }
    
    lora->setDio0Action([]() {});
    
    isInitialized = true;
    Serial.printf("[Meshtastic] LoRa initialized (Node ID: 0x%08X)\n", localNodeID);
    return true;
}

void MeshtasticClient::update() {
    if (!isInitialized) return;
    
    uint8_t buffer[256];
    int state = lora->receive(buffer, sizeof(buffer));
    
    if (state == RADIOLIB_ERR_NONE) {
        int16_t rssi = lora->getRSSI();
        float snr = lora->getSNR();
        size_t length = lora->getPacketLength();
        
        processReceivedPacket(buffer, length, rssi, snr);
    }
}

bool MeshtasticClient::sendRawPacket(const uint8_t* data, size_t length) {
    if (!isInitialized) return false;
    
    if (millis() - lastTransmitTime < transmitInterval) {
        return false;
    }
    
    int state = lora->transmit(const_cast<uint8_t*>(data), length);
    
    if (state == RADIOLIB_ERR_NONE) {
        lastTransmitTime = millis();
        Serial.println("[Meshtastic] Packet transmitted");
        return true;
    }
    
    Serial.printf("[Meshtastic] Transmit failed, code: %d\n", state);
    return false;
}

void MeshtasticClient::processReceivedPacket(const uint8_t* data, size_t length, int16_t rssi, float snr) {
    if (length < 8) return;
    
    MeshPacket packet;
    memcpy(&packet.nodeID, data, 4);
    packet.hopLimit = data[4];
    packet.hopCount = data[5];
    
    size_t payloadLength = length - 6;
    if (payloadLength > sizeof(packet.payload) - 1) {
        payloadLength = sizeof(packet.payload) - 1;
    }
    
    memcpy(packet.payload, data + 6, payloadLength);
    packet.payload[payloadLength] = '\0';
    
    packet.rssi = rssi;
    packet.snr = snr;
    packet.timestamp = millis();
    
    receivedPackets.push_back(packet);
    
    if (std::find(knownNodes.begin(), knownNodes.end(), packet.nodeID) == knownNodes.end()) {
        knownNodes.push_back(packet.nodeID);
        Serial.printf("[Meshtastic] New node discovered: 0x%08X\n", packet.nodeID);
    }
    
    Serial.printf("[Meshtastic] Packet from 0x%08X (RSSI: %d dBm, SNR: %.2f dB)\n", 
                 packet.nodeID, rssi, snr);
}

bool MeshtasticClient::broadcastDetectionAlert(const DetectionAlert& alert) {
    if (!isInitialized) return false;
    
    uint8_t packet[256];
    size_t offset = 0;
    
    memcpy(packet + offset, &localNodeID, 4);
    offset += 4;
    
    packet[offset++] = 3;
    packet[offset++] = 0;
    
    memcpy(packet + offset, &alert, sizeof(DetectionAlert));
    offset += sizeof(DetectionAlert);
    
    return sendRawPacket(packet, offset);
}

bool MeshtasticClient::sendDirectMessage(uint32_t targetNodeID, const char* message) {
    if (!isInitialized) return false;
    
    uint8_t packet[256];
    size_t offset = 0;
    
    memcpy(packet + offset, &localNodeID, 4);
    offset += 4;
    
    packet[offset++] = 3;
    packet[offset++] = 0;
    
    memcpy(packet + offset, &targetNodeID, 4);
    offset += 4;
    
    size_t msgLen = strlen(message);
    if (msgLen > 200) msgLen = 200;
    
    memcpy(packet + offset, message, msgLen);
    offset += msgLen;
    
    return sendRawPacket(packet, offset);
}

uint8_t MeshtasticClient::getReceivedPacketCount() const {
    return receivedPackets.size();
}

MeshPacket MeshtasticClient::getPacket(uint8_t index) const {
    if (index < receivedPackets.size()) {
        return receivedPackets[index];
    }
    return MeshPacket{};
}

void MeshtasticClient::clearPackets() {
    receivedPackets.clear();
}

uint8_t MeshtasticClient::getKnownNodeCount() const {
    return knownNodes.size();
}

uint32_t MeshtasticClient::getNodeID(uint8_t index) const {
    if (index < knownNodes.size()) {
        return knownNodes[index];
    }
    return 0;
}

void MeshtasticClient::setTransmitInterval(uint32_t intervalMs) {
    transmitInterval = intervalMs;
}

int16_t MeshtasticClient::getLastRSSI() const {
    if (receivedPackets.empty()) return 0;
    return receivedPackets.back().rssi;
}

float MeshtasticClient::getLastSNR() const {
    if (receivedPackets.empty()) return 0.0f;
    return receivedPackets.back().snr;
}

uint32_t MeshtasticClient::generateNodeID() {
    uint64_t mac = ESP.getEfuseMac();
    return (uint32_t)(mac & 0xFFFFFFFF);
}
