#ifndef CRSF_PARSER_H
#define CRSF_PARSER_H

#include <Arduino.h>

// CRSF (Crossfire/ExpressLRS) Protocol Constants
#define CRSF_SYNC_BYTE              0xC8
#define CRSF_MAX_PACKET_SIZE        64
#define CRSF_PAYLOAD_SIZE_MAX       60

// CRSF Frame Types
#define CRSF_FRAMETYPE_GPS          0x02
#define CRSF_FRAMETYPE_BATTERY      0x08
#define CRSF_FRAMETYPE_LINK_STATS   0x14
#define CRSF_FRAMETYPE_RC_CHANNELS  0x16
#define CRSF_FRAMETYPE_ATTITUDE     0x1E
#define CRSF_FRAMETYPE_FLIGHT_MODE  0x21

// CRSF Addresses
#define CRSF_ADDRESS_BROADCAST      0x00
#define CRSF_ADDRESS_FLIGHT_CTRL    0xC8
#define CRSF_ADDRESS_RADIO_TX       0xEA
#define CRSF_ADDRESS_RECEIVER       0xEC

struct CRSFPacket {
    uint8_t address;
    uint8_t length;
    uint8_t type;
    uint8_t payload[CRSF_PAYLOAD_SIZE_MAX];
    uint8_t crc;
    bool valid;
};

struct CRSFLinkStats {
    uint8_t uplink_RSSI_1;
    uint8_t uplink_RSSI_2;
    uint8_t uplink_Link_quality;
    int8_t uplink_SNR;
    uint8_t active_antenna;
    uint8_t rf_Mode;
    uint8_t uplink_TX_Power;
    uint8_t downlink_RSSI;
    uint8_t downlink_Link_quality;
    int8_t downlink_SNR;
};

struct CRSFGPS {
    int32_t latitude;   // degE7
    int32_t longitude;  // degE7
    uint16_t groundspeed; // km/h * 10
    uint16_t heading;   // degrees * 100
    uint16_t altitude;  // meters + 1000
    uint8_t satellites;
};

struct CRSFRCChannels {
    uint16_t channels[16]; // 11-bit values (172-1811)
};

class CRSFParser {
private:
    uint8_t rxBuffer[CRSF_MAX_PACKET_SIZE];
    uint8_t rxIndex;
    CRSFPacket currentPacket;
    
    uint8_t calculateCRC(uint8_t* data, uint8_t length);
    bool validateCRC(CRSFPacket* packet);
    
public:
    CRSFParser();
    
    bool parseByte(uint8_t byte);
    bool parseBuffer(uint8_t* data, uint16_t length);
    CRSFPacket getPacket() { return currentPacket; }
    
    // Packet builders for injection
    void buildRCChannels(uint8_t* buffer, uint8_t* length, uint16_t* channels);
    void buildLinkStats(uint8_t* buffer, uint8_t* length, CRSFLinkStats* stats);
    
    // Packet analysis
    bool isLinkStats(CRSFPacket* packet);
    bool isGPS(CRSFPacket* packet);
    bool isRCChannels(CRSFPacket* packet);
    
    CRSFLinkStats parseLinkStats(CRSFPacket* packet);
    CRSFGPS parseGPS(CRSFPacket* packet);
    CRSFRCChannels parseRCChannels(CRSFPacket* packet);
    
    const char* getFrameTypeName(uint8_t type);
    int8_t getRSSIFromLinkStats(CRSFLinkStats* stats);
};

#endif // CRSF_PARSER_H
