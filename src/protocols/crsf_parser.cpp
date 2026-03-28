#include "crsf_parser.h"

CRSFParser::CRSFParser() {
    rxIndex = 0;
    memset(&currentPacket, 0, sizeof(CRSFPacket));
}

uint8_t CRSFParser::calculateCRC(uint8_t* data, uint8_t length) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < length; i++) {
        crc = crc ^ data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0xD5;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

bool CRSFParser::validateCRC(CRSFPacket* packet) {
    uint8_t crcData[2 + packet->length - 2];
    crcData[0] = packet->type;
    memcpy(&crcData[1], packet->payload, packet->length - 2);
    
    uint8_t calculatedCRC = calculateCRC(crcData, packet->length - 1);
    return (calculatedCRC == packet->crc);
}

bool CRSFParser::parseByte(uint8_t byte) {
    if (rxIndex == 0 && byte == CRSF_SYNC_BYTE) {
        rxBuffer[rxIndex++] = byte;
        return false;
    }
    
    if (rxIndex > 0 && rxIndex < CRSF_MAX_PACKET_SIZE) {
        rxBuffer[rxIndex++] = byte;
        
        if (rxIndex >= 3) {
            uint8_t expectedLength = rxBuffer[1] + 2; // Address + Length + Payload + CRC
            
            if (rxIndex >= expectedLength) {
                currentPacket.address = rxBuffer[0];
                currentPacket.length = rxBuffer[1];
                currentPacket.type = rxBuffer[2];
                
                uint8_t payloadLength = currentPacket.length - 2;
                memcpy(currentPacket.payload, &rxBuffer[3], payloadLength);
                currentPacket.crc = rxBuffer[2 + currentPacket.length];
                
                currentPacket.valid = validateCRC(&currentPacket);
                rxIndex = 0;
                return currentPacket.valid;
            }
        }
    }
    
    if (rxIndex >= CRSF_MAX_PACKET_SIZE) {
        rxIndex = 0;
    }
    
    return false;
}

bool CRSFParser::parseBuffer(uint8_t* data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        if (parseByte(data[i])) {
            return true;
        }
    }
    return false;
}

void CRSFParser::buildRCChannels(uint8_t* buffer, uint8_t* length, uint16_t* channels) {
    buffer[0] = CRSF_ADDRESS_FLIGHT_CTRL;
    buffer[1] = 24; // Length (type + 22 bytes payload + CRC)
    buffer[2] = CRSF_FRAMETYPE_RC_CHANNELS;
    
    // Pack 16 channels (11-bit each) into 22 bytes
    uint8_t* payload = &buffer[3];
    memset(payload, 0, 22);
    
    for (uint8_t i = 0; i < 16; i++) {
        uint16_t value = channels[i];
        uint16_t bitOffset = i * 11;
        uint8_t byteOffset = bitOffset / 8;
        uint8_t bitInByte = bitOffset % 8;
        
        payload[byteOffset] |= (value << bitInByte) & 0xFF;
        if (bitInByte + 11 > 8) {
            payload[byteOffset + 1] |= (value >> (8 - bitInByte)) & 0xFF;
        }
        if (bitInByte + 11 > 16) {
            payload[byteOffset + 2] |= (value >> (16 - bitInByte)) & 0xFF;
        }
    }
    
    uint8_t crc = calculateCRC(&buffer[2], 23);
    buffer[25] = crc;
    
    *length = 26;
}

void CRSFParser::buildLinkStats(uint8_t* buffer, uint8_t* length, CRSFLinkStats* stats) {
    buffer[0] = CRSF_ADDRESS_FLIGHT_CTRL;
    buffer[1] = 11; // Length
    buffer[2] = CRSF_FRAMETYPE_LINK_STATS;
    
    buffer[3] = stats->uplink_RSSI_1;
    buffer[4] = stats->uplink_RSSI_2;
    buffer[5] = stats->uplink_Link_quality;
    buffer[6] = stats->uplink_SNR;
    buffer[7] = stats->active_antenna;
    buffer[8] = stats->rf_Mode;
    buffer[9] = stats->uplink_TX_Power;
    buffer[10] = stats->downlink_RSSI;
    buffer[11] = stats->downlink_Link_quality;
    buffer[12] = stats->downlink_SNR;
    
    uint8_t crc = calculateCRC(&buffer[2], 11);
    buffer[13] = crc;
    
    *length = 14;
}

bool CRSFParser::isLinkStats(CRSFPacket* packet) {
    return packet->type == CRSF_FRAMETYPE_LINK_STATS;
}

bool CRSFParser::isGPS(CRSFPacket* packet) {
    return packet->type == CRSF_FRAMETYPE_GPS;
}

bool CRSFParser::isRCChannels(CRSFPacket* packet) {
    return packet->type == CRSF_FRAMETYPE_RC_CHANNELS;
}

CRSFLinkStats CRSFParser::parseLinkStats(CRSFPacket* packet) {
    CRSFLinkStats stats;
    memset(&stats, 0, sizeof(CRSFLinkStats));
    
    if (packet->length >= 11) {
        stats.uplink_RSSI_1 = packet->payload[0];
        stats.uplink_RSSI_2 = packet->payload[1];
        stats.uplink_Link_quality = packet->payload[2];
        stats.uplink_SNR = packet->payload[3];
        stats.active_antenna = packet->payload[4];
        stats.rf_Mode = packet->payload[5];
        stats.uplink_TX_Power = packet->payload[6];
        stats.downlink_RSSI = packet->payload[7];
        stats.downlink_Link_quality = packet->payload[8];
        stats.downlink_SNR = packet->payload[9];
    }
    
    return stats;
}

CRSFGPS CRSFParser::parseGPS(CRSFPacket* packet) {
    CRSFGPS gps;
    memset(&gps, 0, sizeof(CRSFGPS));
    
    if (packet->length >= 17) {
        memcpy(&gps.latitude, &packet->payload[0], 4);
        memcpy(&gps.longitude, &packet->payload[4], 4);
        memcpy(&gps.groundspeed, &packet->payload[8], 2);
        memcpy(&gps.heading, &packet->payload[10], 2);
        memcpy(&gps.altitude, &packet->payload[12], 2);
        gps.satellites = packet->payload[14];
    }
    
    return gps;
}

CRSFRCChannels CRSFParser::parseRCChannels(CRSFPacket* packet) {
    CRSFRCChannels rc;
    memset(&rc, 0, sizeof(CRSFRCChannels));
    
    if (packet->length >= 24) {
        uint8_t* payload = packet->payload;
        
        for (uint8_t i = 0; i < 16; i++) {
            uint16_t bitOffset = i * 11;
            uint8_t byteOffset = bitOffset / 8;
            uint8_t bitInByte = bitOffset % 8;
            
            uint16_t value = payload[byteOffset] >> bitInByte;
            if (bitInByte + 11 > 8) {
                value |= payload[byteOffset + 1] << (8 - bitInByte);
            }
            if (bitInByte + 11 > 16) {
                value |= payload[byteOffset + 2] << (16 - bitInByte);
            }
            
            rc.channels[i] = value & 0x07FF;
        }
    }
    
    return rc;
}

const char* CRSFParser::getFrameTypeName(uint8_t type) {
    switch(type) {
        case CRSF_FRAMETYPE_GPS: return "GPS";
        case CRSF_FRAMETYPE_BATTERY: return "BATTERY";
        case CRSF_FRAMETYPE_LINK_STATS: return "LINK_STATS";
        case CRSF_FRAMETYPE_RC_CHANNELS: return "RC_CHANNELS";
        case CRSF_FRAMETYPE_ATTITUDE: return "ATTITUDE";
        case CRSF_FRAMETYPE_FLIGHT_MODE: return "FLIGHT_MODE";
        default: return "UNKNOWN";
    }
}

int8_t CRSFParser::getRSSIFromLinkStats(CRSFLinkStats* stats) {
    // Convert CRSF RSSI (0-255) to dBm
    // CRSF RSSI formula: dBm = -(RSSI value)
    return -(int8_t)stats->uplink_RSSI_1;
}
