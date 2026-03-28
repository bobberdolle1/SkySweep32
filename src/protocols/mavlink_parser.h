#ifndef MAVLINK_PARSER_H
#define MAVLINK_PARSER_H

#include <Arduino.h>

// MAVLink v1.0 Protocol Constants
#define MAVLINK_STX_V1              0xFE
#define MAVLINK_STX_V2              0xFD
#define MAVLINK_MAX_PAYLOAD_LEN     255

// Common MAVLink Message IDs
#define MAVLINK_MSG_ID_HEARTBEAT            0
#define MAVLINK_MSG_ID_SYS_STATUS           1
#define MAVLINK_MSG_ID_SYSTEM_TIME          2
#define MAVLINK_MSG_ID_GPS_RAW_INT          24
#define MAVLINK_MSG_ID_ATTITUDE             30
#define MAVLINK_MSG_ID_GLOBAL_POSITION_INT  33
#define MAVLINK_MSG_ID_RC_CHANNELS_RAW      35
#define MAVLINK_MSG_ID_COMMAND_LONG         76
#define MAVLINK_MSG_ID_COMMAND_ACK          77

// MAVLink Commands
#define MAV_CMD_NAV_WAYPOINT            16
#define MAV_CMD_NAV_RETURN_TO_LAUNCH    20
#define MAV_CMD_NAV_LAND                21
#define MAV_CMD_DO_SET_MODE             176
#define MAV_CMD_COMPONENT_ARM_DISARM    400

// MAVLink Modes
#define MAV_MODE_MANUAL_DISARMED        0
#define MAV_MODE_MANUAL_ARMED           64
#define MAV_MODE_STABILIZE_DISARMED     80
#define MAV_MODE_STABILIZE_ARMED        144
#define MAV_MODE_AUTO_DISARMED          92
#define MAV_MODE_AUTO_ARMED             220

struct MAVLinkPacket {
    uint8_t magic;              // STX
    uint8_t len;                // Payload length
    uint8_t seq;                // Packet sequence
    uint8_t sysid;              // System ID
    uint8_t compid;             // Component ID
    uint8_t msgid;              // Message ID
    uint8_t payload[255];       // Payload
    uint16_t checksum;          // CRC
    bool valid;
};

struct MAVLinkHeartbeat {
    uint8_t type;               // MAV_TYPE
    uint8_t autopilot;          // MAV_AUTOPILOT
    uint8_t base_mode;          // MAV_MODE
    uint32_t custom_mode;
    uint8_t system_status;      // MAV_STATE
    uint8_t mavlink_version;
};

struct MAVLinkGPS {
    uint64_t time_usec;
    int32_t lat;                // Latitude (degE7)
    int32_t lon;                // Longitude (degE7)
    int32_t alt;                // Altitude (mm)
    uint16_t eph;               // GPS HDOP
    uint16_t epv;               // GPS VDOP
    uint16_t vel;               // Ground speed (cm/s)
    uint16_t cog;               // Course over ground (cdeg)
    uint8_t fix_type;
    uint8_t satellites_visible;
};

class MAVLinkParser {
private:
    uint8_t rxBuffer[280];
    uint16_t rxIndex;
    MAVLinkPacket currentPacket;
    
    uint16_t calculateCRC(uint8_t* data, uint8_t length, uint8_t msgid);
    bool validateChecksum(MAVLinkPacket* packet);
    
public:
    MAVLinkParser();
    
    bool parseBuffer(uint8_t* data, uint16_t length);
    bool parseByte(uint8_t byte);
    MAVLinkPacket getPacket() { return currentPacket; }
    
    // Packet builders for injection attacks
    void buildHeartbeat(uint8_t* buffer, uint8_t* length, uint8_t sysid, uint8_t compid);
    void buildCommandLong(uint8_t* buffer, uint8_t* length, uint16_t command, 
                         uint8_t target_system, uint8_t target_component);
    void buildRTHCommand(uint8_t* buffer, uint8_t* length);
    void buildLandCommand(uint8_t* buffer, uint8_t* length);
    void buildDisarmCommand(uint8_t* buffer, uint8_t* length);
    
    // Packet analysis
    bool isHeartbeat(MAVLinkPacket* packet);
    bool isGPSData(MAVLinkPacket* packet);
    bool isRCChannels(MAVLinkPacket* packet);
    
    MAVLinkHeartbeat parseHeartbeat(MAVLinkPacket* packet);
    MAVLinkGPS parseGPS(MAVLinkPacket* packet);
    
    const char* getMessageName(uint8_t msgid);
    const char* getVehicleType(uint8_t type);
};

#endif // MAVLINK_PARSER_H
