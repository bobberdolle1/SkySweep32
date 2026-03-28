#include "mavlink_parser.h"

// CRC_EXTRA values for MAVLink v1.0 messages
static const uint8_t MAVLINK_CRC_EXTRA[] = {
    50, 124, 137, 0, 237, 217, 104, 119, 0, 0,  // 0-9
    0, 89, 0, 0, 0, 0, 0, 0, 0, 0,              // 10-19
    214, 159, 220, 168, 24, 23, 170, 144, 67, 115, // 20-29
    39, 246, 185, 104, 237, 244, 222, 212, 9, 254, // 30-39
    230, 28, 28, 132, 221, 232, 11, 153, 41, 39,   // 40-49
    78, 196, 0, 0, 15, 3, 0, 0, 0, 0,              // 50-59
    167, 183, 119, 191, 118, 148, 21, 0, 243, 124, // 60-69
    0, 0, 38, 20, 158, 152, 143, 0, 0, 0,          // 70-79
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                  // 80-89
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0                   // 90-99
};

MAVLinkParser::MAVLinkParser() {
    rxIndex = 0;
    memset(&currentPacket, 0, sizeof(MAVLinkPacket));
}

uint16_t MAVLinkParser::calculateCRC(uint8_t* data, uint8_t length, uint8_t msgid) {
    uint16_t crc = 0xFFFF;
    
    for (uint8_t i = 0; i < length; i++) {
        uint8_t tmp = data[i] ^ (uint8_t)(crc & 0xFF);
        tmp ^= (tmp << 4);
        crc = (crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4);
    }
    
    // Add CRC_EXTRA
    if (msgid < sizeof(MAVLINK_CRC_EXTRA)) {
        uint8_t tmp = MAVLINK_CRC_EXTRA[msgid] ^ (uint8_t)(crc & 0xFF);
        tmp ^= (tmp << 4);
        crc = (crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4);
    }
    
    return crc;
}

bool MAVLinkParser::validateChecksum(MAVLinkPacket* packet) {
    uint8_t crcData[6 + packet->len];
    crcData[0] = packet->len;
    crcData[1] = packet->seq;
    crcData[2] = packet->sysid;
    crcData[3] = packet->compid;
    crcData[4] = packet->msgid;
    memcpy(&crcData[5], packet->payload, packet->len);
    
    uint16_t calculatedCRC = calculateCRC(crcData, 5 + packet->len, packet->msgid);
    return (calculatedCRC == packet->checksum);
}

bool MAVLinkParser::parseByte(uint8_t byte) {
    if (rxIndex == 0 && byte == MAVLINK_STX_V1) {
        rxBuffer[rxIndex++] = byte;
        return false;
    }
    
    if (rxIndex > 0 && rxIndex < 280) {
        rxBuffer[rxIndex++] = byte;
        
        if (rxIndex >= 6) {
            uint8_t expectedLength = 8 + rxBuffer[1]; // Header + payload + CRC
            
            if (rxIndex >= expectedLength) {
                currentPacket.magic = rxBuffer[0];
                currentPacket.len = rxBuffer[1];
                currentPacket.seq = rxBuffer[2];
                currentPacket.sysid = rxBuffer[3];
                currentPacket.compid = rxBuffer[4];
                currentPacket.msgid = rxBuffer[5];
                memcpy(currentPacket.payload, &rxBuffer[6], currentPacket.len);
                currentPacket.checksum = rxBuffer[6 + currentPacket.len] | 
                                        (rxBuffer[7 + currentPacket.len] << 8);
                
                currentPacket.valid = validateChecksum(&currentPacket);
                rxIndex = 0;
                return currentPacket.valid;
            }
        }
    }
    
    return false;
}

bool MAVLinkParser::parseBuffer(uint8_t* data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        if (parseByte(data[i])) {
            return true;
        }
    }
    return false;
}

void MAVLinkParser::buildCommandLong(uint8_t* buffer, uint8_t* length, 
                                     uint16_t command, uint8_t target_system, 
                                     uint8_t target_component) {
    buffer[0] = MAVLINK_STX_V1;
    buffer[1] = 33; // Payload length for COMMAND_LONG
    buffer[2] = 0;  // Sequence
    buffer[3] = 255; // System ID (GCS)
    buffer[4] = 190; // Component ID (MAV_COMP_ID_MISSIONPLANNER)
    buffer[5] = MAVLINK_MSG_ID_COMMAND_LONG;
    
    // Payload: param1-7 (float), command (uint16), target_system, target_component, confirmation
    memset(&buffer[6], 0, 33);
    buffer[6 + 28] = command & 0xFF;
    buffer[6 + 29] = (command >> 8) & 0xFF;
    buffer[6 + 30] = target_system;
    buffer[6 + 31] = target_component;
    buffer[6 + 32] = 0; // Confirmation
    
    uint16_t crc = calculateCRC(&buffer[1], 5 + 33, MAVLINK_MSG_ID_COMMAND_LONG);
    buffer[39] = crc & 0xFF;
    buffer[40] = (crc >> 8) & 0xFF;
    
    *length = 41;
}

void MAVLinkParser::buildRTHCommand(uint8_t* buffer, uint8_t* length) {
    buildCommandLong(buffer, length, MAV_CMD_NAV_RETURN_TO_LAUNCH, 1, 1);
}

void MAVLinkParser::buildLandCommand(uint8_t* buffer, uint8_t* length) {
    buildCommandLong(buffer, length, MAV_CMD_NAV_LAND, 1, 1);
}

void MAVLinkParser::buildDisarmCommand(uint8_t* buffer, uint8_t* length) {
    buildCommandLong(buffer, length, MAV_CMD_COMPONENT_ARM_DISARM, 1, 1);
}

void MAVLinkParser::buildHeartbeat(uint8_t* buffer, uint8_t* length, 
                                   uint8_t sysid, uint8_t compid) {
    buffer[0] = MAVLINK_STX_V1;
    buffer[1] = 9; // Payload length for HEARTBEAT
    buffer[2] = 0;
    buffer[3] = sysid;
    buffer[4] = compid;
    buffer[5] = MAVLINK_MSG_ID_HEARTBEAT;
    
    // Payload: type, autopilot, base_mode, custom_mode, system_status, mavlink_version
    buffer[6] = 6;  // MAV_TYPE_GCS
    buffer[7] = 8;  // MAV_AUTOPILOT_INVALID
    buffer[8] = 0;  // Base mode
    buffer[9] = 0;  // Custom mode (4 bytes)
    buffer[10] = 0;
    buffer[11] = 0;
    buffer[12] = 0;
    buffer[13] = 4; // MAV_STATE_ACTIVE
    buffer[14] = 3; // MAVLink version
    
    uint16_t crc = calculateCRC(&buffer[1], 14, MAVLINK_MSG_ID_HEARTBEAT);
    buffer[15] = crc & 0xFF;
    buffer[16] = (crc >> 8) & 0xFF;
    
    *length = 17;
}

bool MAVLinkParser::isHeartbeat(MAVLinkPacket* packet) {
    return packet->msgid == MAVLINK_MSG_ID_HEARTBEAT;
}

bool MAVLinkParser::isGPSData(MAVLinkPacket* packet) {
    return packet->msgid == MAVLINK_MSG_ID_GPS_RAW_INT;
}

bool MAVLinkParser::isRCChannels(MAVLinkPacket* packet) {
    return packet->msgid == MAVLINK_MSG_ID_RC_CHANNELS_RAW;
}

MAVLinkHeartbeat MAVLinkParser::parseHeartbeat(MAVLinkPacket* packet) {
    MAVLinkHeartbeat hb;
    if (packet->len >= 9) {
        hb.type = packet->payload[0];
        hb.autopilot = packet->payload[1];
        hb.base_mode = packet->payload[2];
        memcpy(&hb.custom_mode, &packet->payload[3], 4);
        hb.system_status = packet->payload[7];
        hb.mavlink_version = packet->payload[8];
    }
    return hb;
}

MAVLinkGPS MAVLinkParser::parseGPS(MAVLinkPacket* packet) {
    MAVLinkGPS gps;
    memset(&gps, 0, sizeof(MAVLinkGPS));
    if (packet->len >= 30) {
        memcpy(&gps.time_usec, &packet->payload[0], 8);
        memcpy(&gps.lat, &packet->payload[8], 4);
        memcpy(&gps.lon, &packet->payload[12], 4);
        memcpy(&gps.alt, &packet->payload[16], 4);
        memcpy(&gps.eph, &packet->payload[20], 2);
        memcpy(&gps.epv, &packet->payload[22], 2);
        memcpy(&gps.vel, &packet->payload[24], 2);
        memcpy(&gps.cog, &packet->payload[26], 2);
        gps.fix_type = packet->payload[28];
        gps.satellites_visible = packet->payload[29];
    }
    return gps;
}

const char* MAVLinkParser::getMessageName(uint8_t msgid) {
    switch(msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT: return "HEARTBEAT";
        case MAVLINK_MSG_ID_SYS_STATUS: return "SYS_STATUS";
        case MAVLINK_MSG_ID_GPS_RAW_INT: return "GPS_RAW_INT";
        case MAVLINK_MSG_ID_ATTITUDE: return "ATTITUDE";
        case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: return "GLOBAL_POSITION_INT";
        case MAVLINK_MSG_ID_RC_CHANNELS_RAW: return "RC_CHANNELS_RAW";
        case MAVLINK_MSG_ID_COMMAND_LONG: return "COMMAND_LONG";
        case MAVLINK_MSG_ID_COMMAND_ACK: return "COMMAND_ACK";
        default: return "UNKNOWN";
    }
}

const char* MAVLinkParser::getVehicleType(uint8_t type) {
    switch(type) {
        case 0: return "Generic";
        case 1: return "Fixed Wing";
        case 2: return "Quadrotor";
        case 3: return "Coaxial Heli";
        case 4: return "Helicopter";
        case 5: return "Antenna Tracker";
        case 6: return "GCS";
        case 13: return "Hexarotor";
        case 14: return "Octorotor";
        default: return "Unknown";
    }
}
