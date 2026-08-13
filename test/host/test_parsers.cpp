// Host unit tests for the CRSF and MAVLink protocol parsers.
//
// These target the buffer-safety and correctness fixes for attacker-controlled
// RF input. Build with AddressSanitizer (see Makefile): any out-of-bounds access
// aborts the process, so the "malicious frame" cases double as overflow guards.
//
// Runs on a desktop compiler via the stub Arduino.h in this directory — no ESP32
// toolchain or PlatformIO registry required.

#include "crsf_parser.h"
#include "mavlink_parser.h"
#include "activity_classifier.h"

#include <cstdio>
#include <cstdint>
#include <cstring>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg)                              \
    do {                                              \
        if (cond) {                                   \
            g_pass++;                                 \
        } else {                                      \
            g_fail++;                                 \
            printf("  FAIL: %s\n", msg);              \
        }                                             \
    } while (0)

// Feed a byte stream through a CRSF parser; return true if any frame validated.
static bool feedCRSF(CRSFParser& p, const uint8_t* data, size_t n) {
    bool any = false;
    for (size_t i = 0; i < n; i++) {
        if (p.parseByte(data[i])) any = true;
    }
    return any;
}

static bool feedMAV(MAVLinkParser& p, const uint8_t* data, size_t n) {
    bool any = false;
    for (size_t i = 0; i < n; i++) {
        if (p.parseByte(data[i])) any = true;
    }
    return any;
}

static uint8_t crsfCrc(const uint8_t* data, size_t length) {
    uint8_t crc = 0;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0xD5)
                               : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

static size_t makeLinkStatsFixture(uint8_t* frame, const CRSFLinkStats& stats) {
    frame[0] = CRSF_SYNC_BYTE;
    frame[1] = 12;
    frame[2] = CRSF_FRAMETYPE_LINK_STATS;
    frame[3] = stats.uplink_RSSI_1;
    frame[4] = stats.uplink_RSSI_2;
    frame[5] = stats.uplink_Link_quality;
    frame[6] = static_cast<uint8_t>(stats.uplink_SNR);
    frame[7] = stats.active_antenna;
    frame[8] = stats.rf_Mode;
    frame[9] = stats.uplink_TX_Power;
    frame[10] = stats.downlink_RSSI;
    frame[11] = stats.downlink_Link_quality;
    frame[12] = static_cast<uint8_t>(stats.downlink_SNR);
    frame[13] = crsfCrc(&frame[2], 11);
    return 14;
}

static size_t makeRcChannelsFixture(uint8_t* frame, const uint16_t* channels) {
    frame[0] = CRSF_SYNC_BYTE;
    frame[1] = 24;
    frame[2] = CRSF_FRAMETYPE_RC_CHANNELS;
    memset(&frame[3], 0, 22);
    for (uint8_t i = 0; i < 16; i++) {
        const uint16_t value = channels[i];
        const uint16_t bitOffset = i * 11;
        const uint8_t byteOffset = bitOffset / 8;
        const uint8_t bitInByte = bitOffset % 8;
        frame[3 + byteOffset] |= static_cast<uint8_t>(value << bitInByte);
        if (bitInByte + 11 > 8) {
            frame[4 + byteOffset] |= static_cast<uint8_t>(value >> (8 - bitInByte));
        }
        if (bitInByte + 11 > 16) {
            frame[5 + byteOffset] |= static_cast<uint8_t>(value >> (16 - bitInByte));
        }
    }
    frame[25] = crsfCrc(&frame[2], 23);
    return 26;
}

static uint16_t mavCrc(const uint8_t* data, size_t length, int crcExtra = -1) {
    uint16_t crc = 0xFFFF;
    auto accumulate = [&crc](uint8_t byte) {
        uint8_t tmp = byte ^ static_cast<uint8_t>(crc & 0xFF);
        tmp ^= static_cast<uint8_t>(tmp << 4);
        crc = static_cast<uint16_t>(
            (crc >> 8) ^ (static_cast<uint16_t>(tmp) << 8) ^
            (static_cast<uint16_t>(tmp) << 3) ^ (tmp >> 4));
    };
    for (size_t i = 0; i < length; i++) accumulate(data[i]);
    if (crcExtra >= 0) accumulate(static_cast<uint8_t>(crcExtra));
    return crc;
}

static size_t makeHeartbeatFixture(uint8_t* frame, uint8_t sysid, uint8_t compid) {
    const uint8_t payloadLength = 9;
    const uint8_t messageId = MAVLINK_MSG_ID_HEARTBEAT;
    frame[0] = MAVLINK_STX_V1;
    frame[1] = payloadLength;
    frame[2] = 0;
    frame[3] = sysid;
    frame[4] = compid;
    frame[5] = messageId;
    const uint8_t payload[payloadLength] = {6, 8, 0, 0, 0, 0, 0, 4, 3};
    memcpy(&frame[6], payload, payloadLength);
    const uint16_t crc = mavCrc(&frame[1], 5 + payloadLength, 50);
    frame[15] = static_cast<uint8_t>(crc);
    frame[16] = static_cast<uint8_t>(crc >> 8);
    return 17;
}

static void testCRSF() {
    printf("CRSF parser:\n");

    // 1) Hostile length byte (0xFF) must be rejected, not overflow the 60-byte
    //    payload buffer. Under ASAN the pre-fix code aborts here.
    {
        CRSFParser p;
        uint8_t mal[512];
        mal[0] = CRSF_SYNC_BYTE;
        mal[1] = 0xFF;                 // length -> pre-fix wrapped expectedLength to 1
        for (int i = 2; i < 512; i++) mal[i] = 0xAB;
        bool v = feedCRSF(p, mal, sizeof(mal));
        CHECK(!v, "0xFF length frame must not validate (no overflow)");
    }

    // 2) Length byte 0xFE (the other wrap value) must also be rejected.
    {
        CRSFParser p;
        uint8_t mal[300];
        mal[0] = CRSF_SYNC_BYTE;
        mal[1] = 0xFE;
        for (int i = 2; i < 300; i++) mal[i] = 0x7F;
        bool v = feedCRSF(p, mal, sizeof(mal));
        CHECK(!v, "0xFE length frame must not validate (no overflow)");
    }

    // 3) Over-max payload (length 63 -> payload 61 > 60) must be rejected.
    {
        CRSFParser p;
        uint8_t f[80];
        f[0] = CRSF_SYNC_BYTE;
        f[1] = 63;
        for (int i = 2; i < 80; i++) f[i] = 0;
        bool v = feedCRSF(p, f, sizeof(f));
        CHECK(!v, "payload length above 60 rejected");
    }

    // 4) A valid receive-only LINK_STATS fixture exercises CRC and field offsets.
    {
        CRSFLinkStats stats;
        memset(&stats, 0, sizeof(stats));
        stats.uplink_RSSI_1 = 42;
        stats.downlink_SNR = -7;

        uint8_t buf[64];
        const size_t len = makeLinkStatsFixture(buf, stats);

        CRSFParser p;
        bool v = feedCRSF(p, buf, len);
        CHECK(v, "valid LINK_STATS fixture parses and validates");

        CRSFPacket pkt = p.getPacket();
        CHECK(pkt.valid, "parsed packet flagged valid");
        CHECK(pkt.type == CRSF_FRAMETYPE_LINK_STATS, "parsed type == LINK_STATS");

        CRSFLinkStats out = p.parseLinkStats(&pkt);
        CHECK(out.uplink_RSSI_1 == 42, "payload parses (uplink_RSSI_1 == 42)");
    }
}

static void testMAVLink() {
    printf("MAVLink parser:\n");

    // 1) Valid receive-only HEARTBEAT fixture.
    {
        uint8_t buf[64];
        const size_t len = makeHeartbeatFixture(buf, 1, 1);

        MAVLinkParser p;
        bool v = feedMAV(p, buf, len);
        CHECK(v, "valid heartbeat fixture parses and validates");

        MAVLinkPacket pkt = p.getPacket();
        CHECK(pkt.valid, "heartbeat flagged valid");
        CHECK(pkt.msgid == MAVLINK_MSG_ID_HEARTBEAT, "msgid == HEARTBEAT");
    }

    // 2) Maximum payload (len 255): expectedLength and checksum iteration must
    //    remain wide enough for the complete 263-byte frame.
    {
        const uint8_t payloadLen = 255;
        const uint8_t msgid = MAVLINK_MSG_ID_HEARTBEAT;
        uint8_t frame[8 + 255];
        frame[0] = MAVLINK_STX_V1;
        frame[1] = payloadLen;
        frame[2] = 7;
        frame[3] = 42;
        frame[4] = 1;
        frame[5] = msgid;
        for (uint16_t i = 0; i < payloadLen; i++) {
            frame[6 + i] = static_cast<uint8_t>(i * 3 + 1);
        }

        const uint16_t crc = mavCrc(&frame[1], 5 + payloadLen, 50);
        frame[6 + payloadLen] = static_cast<uint8_t>(crc);
        frame[7 + payloadLen] = static_cast<uint8_t>(crc >> 8);

        MAVLinkParser p;
        bool v = feedMAV(p, frame, sizeof(frame));
        CHECK(v, "len=255 frame validates without length wrap");

        MAVLinkPacket pkt = p.getPacket();
        CHECK(pkt.len == 255, "parsed len == 255");
        CHECK(pkt.msgid == MAVLINK_MSG_ID_HEARTBEAT, "parsed supported msgid");
    }

    // 3) Message IDs without an explicit common-dialect CRC_EXTRA entry are
    //    unsupported. Frames omitting CRC_EXTRA must not be accepted as MAVLink,
    //    including both an in-range table hole and an out-of-range ID.
    {
        const uint8_t unsupportedIds[] = {3, 200};
        for (uint8_t msgid : unsupportedIds) {
            uint8_t frame[9] = {MAVLINK_STX_V1, 1, 9, 1, 1, msgid, 0x5A, 0, 0};
            const uint16_t crc = mavCrc(&frame[1], 6);
            frame[7] = static_cast<uint8_t>(crc);
            frame[8] = static_cast<uint8_t>(crc >> 8);

            MAVLinkParser p;
            CHECK(!feedMAV(p, frame, sizeof(frame)),
                  "unsupported msgid without CRC_EXTRA is rejected");
        }
    }

    // 4) Truncated large frame must not be accepted or over-read.
    {
        MAVLinkParser p;
        uint8_t frag[12];
        frag[0] = MAVLINK_STX_V1;
        frag[1] = 255;
        for (int i = 2; i < 12; i++) frag[i] = 0x11;
        bool v = feedMAV(p, frag, sizeof(frag));
        CHECK(!v, "12-byte fragment of a 263-byte frame is not accepted");
    }

    // 5) parseHeartbeat must zero-initialize its result on a short payload
    //    (previously returned an uninitialized struct).
    {
        MAVLinkPacket pkt;
        memset(&pkt, 0xAA, sizeof(pkt));  // fill with junk
        pkt.len = 5;                      // below the 9 bytes heartbeat needs
        pkt.msgid = MAVLINK_MSG_ID_HEARTBEAT;

        MAVLinkParser p;
        MAVLinkHeartbeat hb = p.parseHeartbeat(&pkt);
        CHECK(hb.type == 0 && hb.autopilot == 0 && hb.base_mode == 0 &&
                  hb.custom_mode == 0 && hb.system_status == 0 && hb.mavlink_version == 0,
              "parseHeartbeat zero-inits on short payload");
    }
}

// Exercises the CRSF/MAVLink field-extraction helpers directly: correct decoding
// on full-length payloads and safe zeroed output on short ones (bounds guards).
static void testParseHelpers() {
    printf("Parse helpers (bounds + extraction):\n");

    // CRSF RC-channel fixture through parse (11-bit pack/unpack + CRC).
    {
        uint16_t in[16];
        for (int i = 0; i < 16; i++) in[i] = static_cast<uint16_t>(172 + i * 20);

        uint8_t buf[64];
        const size_t len = makeRcChannelsFixture(buf, in);

        CRSFParser p;
        bool v = feedCRSF(p, buf, len);
        CHECK(v, "RC-channels fixture parses and validates");

        CRSFPacket pkt = p.getPacket();
        CRSFRCChannels rc = p.parseRCChannels(&pkt);
        bool allMatch = true;
        for (int i = 0; i < 16; i++) if (rc.channels[i] != in[i]) allMatch = false;
        CHECK(allMatch, "RC channels parse losslessly (11-bit pack/unpack)");
    }

    // CRSF GPS: extracts on length >= 17, zeroed on short length.
    {
        CRSFPacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.length = 17;
        pkt.payload[14] = 9;  // satellites
        CRSFParser p;
        CRSFGPS gps = p.parseGPS(&pkt);
        CHECK(gps.satellites == 9, "CRSF parseGPS extracts satellites on full payload");

        CRSFPacket shortPkt;
        memset(&shortPkt, 0, sizeof(shortPkt));
        shortPkt.length = 10;             // below 17
        shortPkt.payload[14] = 9;
        CRSFGPS g2 = p.parseGPS(&shortPkt);
        CHECK(g2.satellites == 0 && g2.latitude == 0, "CRSF parseGPS zeroed on short payload");
    }

    // MAVLink GPS: extracts on len >= 30, zeroed on short len.
    {
        MAVLinkPacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.len = 30;
        pkt.payload[28] = 3;   // fix_type
        pkt.payload[29] = 11;  // satellites_visible
        MAVLinkParser p;
        MAVLinkGPS gps = p.parseGPS(&pkt);
        CHECK(gps.fix_type == 3 && gps.satellites_visible == 11,
              "MAVLink parseGPS extracts fix/sats on full payload");

        MAVLinkPacket shortPkt;
        memset(&shortPkt, 0, sizeof(shortPkt));
        shortPkt.len = 20;     // below 30
        shortPkt.payload[29] = 11;
        MAVLinkGPS g2 = p.parseGPS(&shortPkt);
        CHECK(g2.satellites_visible == 0 && g2.lat == 0, "MAVLink parseGPS zeroed on short payload");
    }

    // MAVLink heartbeat extraction on a full payload.
    {
        MAVLinkPacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.len = 9;
        pkt.msgid = MAVLINK_MSG_ID_HEARTBEAT;
        pkt.payload[0] = 2;   // type = quadrotor
        pkt.payload[1] = 12;  // autopilot
        pkt.payload[7] = 4;   // system_status
        pkt.payload[8] = 3;   // mavlink_version
        MAVLinkParser p;
        MAVLinkHeartbeat hb = p.parseHeartbeat(&pkt);
        CHECK(hb.type == 2 && hb.autopilot == 12 && hb.system_status == 4 && hb.mavlink_version == 3,
              "MAVLink parseHeartbeat extracts fields on full payload");
    }
}


// Deterministic PRNG (xorshift32) so the fuzz run is reproducible — no time or
// std::random seeding, same sequence every build.
static uint32_t g_rng = 0x1a2b3c4du;
static uint32_t xrand() {
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 17;
    g_rng ^= g_rng << 5;
    return g_rng;
}

// Feed random and adversarial byte streams through both parsers. The parsers
// decode attacker-controlled RF, so the property under test is simply: no input
// causes an out-of-bounds access. Built with AddressSanitizer, any OOB aborts
// the process, so reaching the end == pass.
static void testFuzz() {
    printf("Fuzz (ASan/UBSan guard OOB on arbitrary input):\n");

    // 1) Unstructured byte stream through long-lived parsers (they self-resync).
    {
        CRSFParser cp;
        MAVLinkParser mp;
        for (int i = 0; i < 300000; i++) {
            uint8_t b = (uint8_t)(xrand() & 0xFF);
            cp.parseByte(b);
            mp.parseByte(b);
        }
    }

    // 2) Structured frames: valid sync + random length byte (hits 0xFE/0xFF and
    //    every boundary) + random payload, each into a fresh parser.
    for (int iter = 0; iter < 80000; iter++) {
        uint8_t frame[320];
        int n = 2 + (int)(xrand() % 300);
        frame[0] = CRSF_SYNC_BYTE;
        frame[1] = (uint8_t)(xrand() & 0xFF);
        for (int i = 2; i < n; i++) frame[i] = (uint8_t)(xrand() & 0xFF);
        CRSFParser p;
        for (int i = 0; i < n; i++) p.parseByte(frame[i]);
    }
    for (int iter = 0; iter < 80000; iter++) {
        uint8_t frame[320];
        int n = 2 + (int)(xrand() % 300);
        frame[0] = MAVLINK_STX_V1;
        frame[1] = (uint8_t)(xrand() & 0xFF);
        for (int i = 2; i < n; i++) frame[i] = (uint8_t)(xrand() & 0xFF);
        MAVLinkParser p;
        for (int i = 0; i < n; i++) p.parseByte(frame[i]);
    }

    CHECK(true, "fuzz completed without a sanitizer abort");
}

static void testActivityClassification() {
    printf("Passive activity classification:\n");
    const ActivityThresholds thresholds = {35, 50, 70, 85};

    CHECK(classifyActivityLevel(0, thresholds) == ACTIVITY_NONE,
          "below low threshold is NONE");
    CHECK(classifyActivityLevel(34, thresholds) == ACTIVITY_NONE,
          "value immediately below low threshold is NONE");
    CHECK(classifyActivityLevel(35, thresholds) == ACTIVITY_LOW,
          "low threshold is inclusive");
    CHECK(classifyActivityLevel(49, thresholds) == ACTIVITY_LOW,
          "value below medium remains LOW");
    CHECK(classifyActivityLevel(50, thresholds) == ACTIVITY_MEDIUM,
          "medium threshold is inclusive");
    CHECK(classifyActivityLevel(70, thresholds) == ACTIVITY_HIGH,
          "high threshold is inclusive");
    CHECK(classifyActivityLevel(85, thresholds) == ACTIVITY_CRITICAL,
          "critical threshold is inclusive");
    CHECK(classifyActivityLevel(100, thresholds) == ACTIVITY_CRITICAL,
          "value above critical remains CRITICAL");
}

int main() {
    printf("== SkySweep32 host parser tests ==\n");
    testCRSF();
    testMAVLink();
    testParseHelpers();
    testActivityClassification();
    testFuzz();
    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
