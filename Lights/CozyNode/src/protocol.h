#pragma once
#include <cstdint>

// ── Packet Types ──────────────────────────────────────────────────────────────
enum class PacketType : uint8_t {
    Stream    = 0x01,
    Config    = 0x02,
    Telemetry = 0x03,
};

// ── Telemetry Status ──────────────────────────────────────────────────────────
enum class TelemetryStatus : uint8_t {
    StreamLocked = 1 << 0,
    IndexSet     = 1 << 1,
    StreamActive = 1 << 2,
};

// ── Threshold-encoded sensor readings ─────────────────────────────────────────
enum class BatteryLevel : uint8_t { Full = 0, OK = 1, Low = 2 };
enum class TempLevel    : uint8_t { OK   = 0, Hot = 1 };

// ── Packed packet structs ──────────────────────────────────────────────────────
#pragma pack(push, 1)

// Every packet begins with this 4-byte header.
struct PacketHeader {
    uint8_t    magic[3];
    PacketType type;
};

// Stream packet — broadcast, high-rate, no reply.
// Payload is a flat uint8 array immediately following the header.
// Each node reads raw_data[ offset .. offset + span - 1 ] for its assigned channels,
// where offset is the absolute byte offset stored on the node from its last config.
struct StreamPacket {
    PacketHeader header;
    // uint8_t payload[] follows — variable length, not modelled in the struct
};

// Config packet — broadcast, addressed at the application layer via target_mac.
// All nodes receive it; only the node whose MAC matches target_mac acts on it.
// No peer registration required on either side.
//
// Sent periodically as a reminder — handles fixtures that reset mid-show.
// The node replies with a telemetry broadcast if it has something to report.
//
// offset is the absolute byte offset from the start of the raw stream packet.
// Minimum valid offset is sizeof(PacketHeader) = 4.
// offset == 0 && span == 0 is the canonical unassigned / reset sentinel.
//
// The master's identity is not embedded — the node reads src_addr from the
// ESP-NOW receive callback and stores it for the telemetry reply.
struct ConfigPacket {
    PacketHeader header;
    uint8_t      target_mac[6];        // MAC of the intended recipient
    uint8_t      version;
    uint16_t     offset;               // Absolute byte offset into raw stream packet
    uint8_t      span;                 // Number of consecutive channels this fixture owns
    uint8_t      warm_white_enabled;   // 1 = WW channel active, 0 = unused
    uint8_t      uv_enabled;           // 1 = UV channel active, 0 = unused
    uint8_t      servo_enabled;        // 1 = servo channels active, 0 = LED-only fixture
    uint8_t      servo_timeout_enabled;// 1 = cut servo power after inactivity, 0 = always on
    uint8_t      diag_override_r;      // \
    uint8_t      diag_override_g;      //  > Raw RGB colour for diag LED override
    uint8_t      diag_override_b;      // /
    uint8_t      diag_override_t;      // Duration: 0 = cancel/no override, 1–254 = seconds, 255 = indefinite
};

// Telemetry reply — broadcast, sent lazily in response to a config packet.
// Master identifies the sender via src_addr in the ESP-NOW receive callback.
// Master does not wait for it — fire and forget.
struct TelemetryPacket {
    PacketHeader    header;
    uint8_t         version;   // Node's actual running firmware version
    TelemetryStatus status;
    BatteryLevel    battery;
    TempLevel       temp;
};

#pragma pack(pop)

// ── Packet size constants ──────────────────────────────────────────────────────
constexpr uint8_t STREAM_HEADER_SIZE    = sizeof(StreamPacket);   // = sizeof(PacketHeader) = 4
constexpr uint8_t CONFIG_PACKET_SIZE    = sizeof(ConfigPacket);
constexpr uint8_t TELEMETRY_PACKET_SIZE = sizeof(TelemetryPacket);

// ── Inline helpers ─────────────────────────────────────────────────────────────

inline bool packet_magic_valid(const uint8_t* data, const uint8_t magic[3]) {
    return data[0] == magic[0] && data[1] == magic[1] && data[2] == magic[2];
}

inline PacketType packet_type(const uint8_t* data) {
    return static_cast<PacketType>(data[3]);
}

// Returns true if an offset/span pair represents a valid assigned slot.
// offset=0 span=0 is the canonical unassigned sentinel.
inline bool slot_is_assigned(uint16_t offset, uint8_t span) {
    return span > 0 && offset >= sizeof(PacketHeader);
}
