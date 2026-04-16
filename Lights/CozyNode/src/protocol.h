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

// ── Slot ──────────────────────────────────────────────────────────────────────
// Defines what section of a stream packet a node engine reads.
// start: absolute byte offset from the beginning of the raw stream packet.
// span:  number of consecutive channel bytes.
//
// Before reading, a node validates: packet_len >= start + span.
// span == 0 means this engine has no channels assigned (inactive).
struct Slot {
    uint16_t start;
    uint8_t  span;
};

// ── Packed packet structs ──────────────────────────────────────────────────────
#pragma pack(push, 1)

// Every packet begins with this 4-byte header.
struct PacketHeader {
    uint8_t    magic[3];
    PacketType type;
};

// Stream packet — broadcast, high-rate, no reply.
// Payload is a flat uint8 array immediately following the header.
// Each engine on the node reads raw_data[ slot.start .. slot.start + slot.span - 1 ]
// using the slots assigned in its last config.
struct StreamPacket {
    PacketHeader header;
    // uint8_t payload[] follows — variable length, not modelled in the struct
};

// Config packet — broadcast, addressed at application layer via target_mac.
// All nodes receive it; only the node whose MAC matches target_mac acts on it.
// No peer registration required on either side.
//
// Sent periodically as a reminder — handles fixtures that reset mid-show.
// On receiving a config, the node stores the slots, replies with telemetry,
// and lazily registers the sender MAC (from ESP-NOW src_addr) for the reply.
//
// beams_slot.span == 0  → LED engine inactive for this fixture.
// servos_slot.span == 0 → servo engine inactive for this fixture.
struct ConfigPacket {
    PacketHeader header;
    uint8_t      target_mac[6];
    uint8_t      version;
    Slot         beams_slot;
    Slot         servos_slot;
    uint8_t      servo_timeout_enabled;  // 1 = cut servo power after inactivity, 0 = always on
    uint8_t      diag_override_r;        // \
    uint8_t      diag_override_g;        //  > Raw RGB override for diag LED
    uint8_t      diag_override_b;        // /
    uint8_t      diag_override_t;        // 0 = cancel override, 1–254 = seconds, 255 = indefinite
};

// Telemetry reply — broadcast, sent lazily in response to a config packet.
// Master identifies the sender via src_addr in the ESP-NOW receive callback.
// Master does not wait for it — fire and forget.
struct TelemetryPacket {
    PacketHeader    header;
    uint8_t         version;
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

// True if a slot has channels assigned and starts within valid packet territory.
inline bool slot_is_active(const Slot& slot) {
    return slot.span > 0 && slot.start >= sizeof(PacketHeader);
}

// True if the stream packet is long enough to satisfy this slot's read range.
// Drop the packet if this returns false.
inline bool slot_fits_packet(const Slot& slot, uint16_t packet_len) {
    return packet_len >= (uint16_t)slot.start + slot.span;
}
