# ESP-NOW Stage Light System — Design Specification
**Version:** 0.3-draft  
**Status:** Pre-implementation design  

Name: CozyNode v2. Simply CozyNode or Node.
Yes, it is an MLP: FiM reference to Cozy Glow

---

## 1. Overview

A wireless stage lighting system built around ESP32-WROOM modules, communicating via ESP-NOW broadcast. Each fixture is a self-contained unit with its own MCU, running identical firmware and self-configuring from a master controller at boot. Designed for theatre, live performance, and music-synced shows.

### Design Philosophy
- **One MCU per fixture** — no shared failure domains
- **Identical firmware on all nodes** — configuration is data, not code
- **Fire-and-forget protocol** — no blocking, no three-way handshakes
- **Self-healing** — lights recover autonomously from power loss, signal loss, or master swap
- **Minimal hardware** — direct solder construction, no connectors, no PCBs required

---

## 2. Hardware

### 2.1 Target Module
**ESP32-WROOM (NodeMCU-32S form factor)**  
Dual-core 240MHz, 520KB SRAM, integrated 2.4GHz WiFi (ESP-NOW), 34 GPIOs.

### 2.2 Pin Allocation

| GPIO | Index | Row | Function | Notes |
|------|-------|-----|----------|-------|
| 36 (SVP) | — | 1 | Battery ADC | ADC1_CH0, input-only, WiFi-safe |
| 39 (SVN) | — | 1 | Temp ADC | ADC1_CH3, input-only, WiFi-safe |
| 32 | — | 1 | LED CH0 — Red | ADC1_CH4, PWM via LEDC |
| 33 | — | 1 | LED CH1 — Green | ADC1_CH5, PWM via LEDC |
| 25 | — | 1 | LED CH2 — Blue | DAC1, PWM via LEDC |
| 26 | — | 1 | LED CH3 — Cold White | DAC2, PWM via LEDC |
| 27 | — | 1 | LED CH4 — Warm White / UV | PWM via LEDC |
| 13 | — | 1 | LED CH5 — UV / spare | PWM via LEDC |
| 16 | — | 2 | Servo CH0 — Pan | LEDC 50Hz timer |
| 17 | — | 2 | Servo CH1 — Tilt | LEDC 50Hz timer |
| 19 | — | 2 | WS2812 Status LED | Timing-sensitive, isolated |
| 21 | — | 2 | Expansion | I2C SDA / future use |
| 22 | — | 2 | Expansion | I2C SCL / future use |
| 23 | — | 2 | Expansion | SPI MOSI / future use |

#### Excluded Pins (and why)
| GPIO | Reason |
|------|--------|
| 0, 2, 5, 12, 15 | Strapping pins — affect boot mode |
| 1, 3 | UART0 TX/RX — Serial monitor |
| 6–11 | Internal SPI flash — never touch |
| 34, 35 | Input-only — reserved if needed for future ADC |
| 4, 18 | Available but deferred to expansion |

### 2.3 LEDC Timer Configuration

| Timer | Frequency | Channels | Purpose |
|-------|-----------|----------|---------|
| Timer A | 40 kHz | CH0–CH5 (GPIO 32,33,25,26,27,13) | LED PWM — 10-bit resolution, audio rate, camera-safe |
| Timer B | 50 Hz | CH6–CH7 (GPIO 16,17) | Servo PWM |

> 40kHz gives 10-bit PWM resolution at audio rate. No rolling shutter banding on cameras. Verify empirically against specific camera systems in use.

### 2.4 Construction Notes
- Direct solder to ESP32 module pads
- Heatshrink all joints
- Strain relief wire bundle to chassis before the solder joint
- No connectors — empirically unreliable in live performance environments
- The weakest point should always be the wire, not the junction

### 2.5 Power
- All fixtures support battery operation
- Mini moving heads (SG90 servos) — battery primary
- Larger fixed fixtures — wall power preferred, battery capable
- Servos power down after repositioning; gear friction holds position
- Battery level reported via ADC1 (GPIO 36) — threshold-based: Full / OK / Low

---

## 3. Fixture Variants

All variants run **identical firmware**. Capabilities are declared to the master at config time, not compiled in.

| Variant | Channels | Span | Notes |
|---------|----------|------|-------|
| RGBC | R, G, B, CW | 4 | Mini lights, basic all-in-one LED |
| RGBCW | R, G, B, CW, WW | 5 | Standard wash fixture |
| RGBCW + Servo | R, G, B, CW, WW, Pan, Tilt | 7 | Moving head, full white |
| RGBC + Servo | R, G, B, CW, Pan, Tilt | 6 | Mini moving head |
| RGBCWU + Servo | R, G, B, CW, WW, UV, Pan, Tilt | 8 | Full package |

### Channel Ordering (fixed, firmware-interpreted)
```
[ R ][ G ][ B ][ CW ][ WW ][ UV ][ Pan ][ Tilt ]
  0    1    2    3     4     5     6      7
```
Unused trailing channels are simply ignored by the node. The master allocates only the span it knows the fixture needs.

> UV channel exists in hardware on all units. It is excluded from the default stream to save packet bytes. Enabled per fixture via master config.

---

## 4. Protocol

### 4.1 ESP-NOW Configuration
- **Mode:** Station (STA)
- **Version:** v1.0 compatible (v2.0 if IDF supports it — transparent upgrade)
- **Channel:** One of three pre-agreed channels (interference avoidance)
- **Encryption:** None (magic word provides basic discrimination)
- **Max packet:** 250 bytes (v1.0) / 1470 bytes (v2.0)

### 4.2 Packet Types

#### Stream Packet (Broadcast)
High-rate lighting data. Sent by master, received by all nodes.  
Nodes **never reply** to stream packets.

```
[ Magic Word (3B) ][ Packet Type (1B) ][ Channel (1B) ][ Payload (N bytes) ]
```

Payload is a flat array of uint8 values, one per channel slot, indexed from 0.  
Each fixture reads `payload[offset]` through `payload[offset + span - 1]`.

When the master has no active lighting data (computer idle), it may transmit at a reduced rate or send a minimal `[magic][type]` frame with no payload. This is not a distinct packet type — just the stream at low or zero content. Purpose: keep nodes locked to channel, prevent recovery mode entry.

#### Config Packet (Unicast)
Sent by master to a specific fixture MAC. Contains fixture assignment and capabilities.

```
[ Magic (3B) ][ Type (1B) ][ Version (1B) ][ Offset (2B) ][ Span (1B) ][ Flags (1B) ]
```

- **Offset** is absolute from packet start (so offset < 4 is invalid — inside magic word). Parser subtracts header size internally.
- **Version** is for compatibility tracking — mismatch is logged, not enforced.
- **Flags** encode active channels: UV, WW, Servo, DiagLED override, etc.
- Sent periodically as a reminder — handles fixtures that reset mid-show.

#### Telemetry Reply (Unicast)
Sent by node in response to a config packet. Master does not wait for it — fire and forget.

```
[ Magic (3B) ][ Type (1B) ][ Version (1B) ][ Status (1B) ][ Battery (1B) ][ Temp (1B) ]
```

- **Version** is the node's actual running firmware version — does not need to match master's expectation, just visible for tracking.
- **Status** bitmask: stream_locked, index_set, stream_active.
- **Battery / Temp** are threshold-encoded (Full/OK/Low, OK/Hot) not raw ADC.
- If the node can reply, it is by definition in a functional state — status is mainly useful for maintenance logging.

### 4.3 Capacity
At 250 byte packets with ~10 byte header overhead:
- **~30 fixtures** at 8 channels each per broadcast burst (v1.0)
- **~180 fixtures** per burst (v2.0)
- At 40Hz broadcast rate: ~50 KB/s — well within ~214 Kbps real-world ESP-NOW throughput

### 4.4 Master–Computer Link
- UART/USB at 115200 baud or higher
- Computer sends MIDI or equivalent control data
- Master acts as a **bridge**, not a self-contained controller
- Computer handles all MIDI clock, BPM sync, cue management
- Master translates to ESP-NOW stream packets

---

## 5. Node State Machine

All beam output is managed exclusively through the **beams engine**, which handles interpolation, gamma correction, and LEDC hardware. Firmware never writes directly to beam channels.

```
                    ┌─────────────┐
          power on  │             │
         ──────────▶│  BOOT       │ DiagLED: white flash (self-test)
                    │             │ Beams: dark (never touched)
                    └──────┬──────┘
                           │ index = IMPOSSIBLE (0xFF)
                           ▼
                    ┌─────────────┐
                    │  SEARCHING  │◀─────────────────────┐
                    │             │  signal lost          │
                    │ DiagLED:    │  for 1–2 seconds      │
                    │ red breathe │                       │
                    └──────┬──────┘                       │
                           │ magic word heard              │
                           │ on any of 3 channels          │
                           ▼                               │
                    ┌──────────────┐                      │
                    │  LOCKED      │  signal lost ─────────┘
                    │  (no index)  │
                    │ DiagLED:     │
                    │ faint green  │
                    │ breathe      │
                    └──────┬───────┘
                           │ config unicast received
                           │ index != IMPOSSIBLE
                           ▼
                    ┌─────────────┐
                    │   ACTIVE    │  signal lost ──────────┐
                    │             │                        │
                    │ DiagLED:off │                        ▼
                    │ Beams: live │              ┌──────────────────┐
                    └──────┬──────┘              │  SIGNAL LOST     │
                           │                     │  (had config)    │
                           │ config reminder      │                  │
                           ▼                     │ DiagLED: neon    │
                    reply telemetry              │ orange breathe   │
                    lazily register              │                  │
                    master MAC                   │ Beams: hold last │
                                                 │ state briefly,   │
                                                 │ then ramp to 0   │
                                                 │ over ~2 seconds  │
                                                 │ via beams engine │
                                                 │                  │
                                                 │ Servos: off once │
                                                 │ beams reach 0    │
                                                 └────────┬─────────┘
                                                          │
                                          ┌───────────────┴──────────────┐
                                          │ stream resumes               │ empty stream only
                                          ▼                               ▼
                                       ACTIVE                     stay dark, DiagLED off
                                  ramp to new values              (camouflaged, ready)
                                  via beams engine
```

### DiagLED (WS2812) Status

DiagLED is the technician-facing status indicator. On boot it flashes white briefly to confirm all three WS2812 channels work — **before** its colour-coded status meaning applies. This validates the diagnostic tool itself.

**Beams (main LED channels) are never driven by firmware logic.** They produce no output at boot, on error, or on reset — only stream data can illuminate them. A fixture that fails or resets must **fail dark**.

#### Status Truth Table

| Lost signal | Configured (has index) | State | DiagLED colour | Pattern |
|-------------|----------------------|-------|----------------|---------|
| ✓ | ✗ | Boot / searching | Red | Slow breathe (50–100%) |
| ✓ | ✓ | Had config, lost stream | Neon orange | Slow breathe (50–100%) |
| ✗ | ✗ | Locked, unassigned | Faint green | Slow breathe (50–100%) |
| ✗ | ✓ | Active, show mode | Off | — |

> Slow breathe (oscillating between 50% and 100% brightness) rather than full on/off pulse — less distracting on stage, still detectable, and confirms CPU is running.

> Solid red would indicate a hard fault — theoretically possible but no known trigger cases yet.

> DiagLED colour and duration can be overridden via unicast config packet (1B colour + 1B timer, 0 = hold indefinitely). Useful for locate commands and edge cases.

---

## 6. Channel Hunting (Interference Avoidance)

Three ESP-NOW channels are agreed upon in firmware (e.g. 1, 6, 11 — standard non-overlapping WiFi channels).

On entering SEARCHING state:
1. Listen on current channel for 1–2 seconds
2. If magic word heard → lock channel
3. If not → advance to next channel, repeat

Master can switch channels freely. Nodes will re-hunt and re-lock automatically. No coordination required.

---

## 7. Master Peer Management (Node Side)

Nodes do not pre-register the master MAC. On receiving a config unicast:

```
if master_mac not in peer_list:
    esp_now_add_peer(master_mac)
    store master_mac in RAM
reply with telemetry
```

- Master MAC stored in RAM only — clears on reset (intentional)
- If master changes mid-show, node lazily re-registers new master on next config reminder
- At most one missed telemetry reply during master swap
- Hot-swappable master — no fixture reconfiguration needed

---

## 8. Fixture Identification

### Physical
Each fixture carries a sticker with:
```
MAC: AA:BB:CC:DD:EE:FF
TYPE: RGBCW_SERVO
FW: 1.0.0
```
And optionally a QR code encoding the same data as JSON for fast import via phone app.

### Software Bookkeeping (Master / PC)
```json
[
  {
    "mac": "AA:BB:CC:DD:EE:FF",
    "type": "RGBCW_SERVO",
    "firmware": "1.0.0",
    "label": "stage-left-wash",
    "span": 7,
    "offset": 0
  }
]
```

- Maintained by operator, imported into lighting software before show
- Master uses this to build config packets and stream slot assignments
- Offset + span define each fixture's slice of the broadcast payload
- Overlapping or unused slots are harmless — nodes ignore what they don't need

---

## 9. Beams Engine

All beam output is exclusively routed through the beams engine. **Firmware never writes directly to LEDC channels.** This includes boot, error, recovery, and signal loss states — the beams engine is the only path to beam output.

### Responsibilities
- **Interpolation** — smooth fading between target values, simulating incandescent warmup/decay
- **Gamma correction** — perceptual linearisation of PWM output
- **LEDC hardware abstraction** — maps logical channels to GPIO/timer configuration
- **Signal loss fade** — on comms loss, ramps all channels to zero over ~2 seconds (empirically tuned)
- **Recovery ramp** — on stream resume with data, ramps from zero to received values
- **Fail dark guarantee** — beams engine initialises all channels at zero; only stream data can produce output

### Fail Dark Policy
A fixture that fails, resets, or loses power must **fail dark**. This is unconditional:
- Beams produce no output at boot
- Beams produce no output on error or reset
- A fixture cycling power on stage will not strobe the audience

This is preferable to a fixture freezing at full brightness — dark is always recoverable, frozen-bright may not be.

### Signal Loss Behaviour
1. On comms loss: hold last beam state briefly (allow fast recovery without visible change)
2. If no reconnect: ramp all beams to zero via beams engine over ~2 seconds
3. Once all beams reach zero: disable servo PWM (friction holds last position)
4. DiagLED → neon orange breathe

### Recovery Behaviour
- **Stream resumes with data** → ramp beams to received values via beams engine, re-enable servos on first position data
- **Empty stream / heartbeat only** → stay dark, DiagLED off (camouflaged, ready for action)
- Servos only re-enable on actual position data — not on empty stream

> The ~2 second ramp-down is a starting point. Tune empirically in performance conditions.

### Servo Behaviour
- Update only when received pan/tilt delta exceeds deadband threshold (TBD)
- After repositioning, servo PWM is **disabled** — gear friction holds position
- Critical for battery-operated mini moving heads (SG90)
- Larger fixtures use proportionally larger servos; same power-off policy applies

---

## 10. Telemetry (TBD — may defer to post-v1)

Telemetry is **optional and non-blocking**. Reported only in reply to config unicast.

| Field | Resolution | Thresholds |
|-------|-----------|------------|
| Battery | 8-bit ADC reading | Full / OK / Low |
| Temperature | 8-bit ADC reading | OK / Hot |
| Status | Bitmask | locked, index_set, stream_active, fw_version |

Master logs telemetry if received, discards if not. No retries, no ACK required.

---

## 11. Timing Budget

| Parameter | Value | Notes |
|-----------|-------|-------|
| Stream rate | 30–40 Hz | Sufficient for music-sync |
| Keepalive (idle) | Reduced rate or minimal frame | No distinct packet type |
| Config reminder rate | Very slow, interlaced | One fixture per N stream frames |
| Recovery timeout | 1–2 seconds | Before entering channel hunt |
| Signal loss fade | ~2 seconds | Empirically tuned, via beams engine |
| Servo update | On delta only | Deadband TBD in firmware |
| Servo power-off | After beams reach zero | On signal loss only |
| Audio delay compensation | ~10ms | Delay audio in DAW to sync |
| LED PWM | 40 kHz, 10-bit | Camera-safe, audio rate |

---

## 12. Open Questions

- [ ] Exact deadband threshold for servo updates
- [ ] Servo power-off delay after last intentional move (normal operation)
- [ ] Whether to include telemetry in v1 firmware or defer
- [ ] QR code app — native or web-based?
- [ ] Config reminder interlace rate (every N frames?)
- [ ] WW/UV pin assignment per fixture variant (CH4 = WW or UV depending on type)
- [ ] Exact magic word value (3 bytes, hardcoded in firmware)
- [ ] Three agreed channel numbers
- [ ] Signal loss fade duration — empirical tuning required (~2s starting point)

---

*This document reflects design decisions made during pre-implementation discussion. Implementation has not begun.*
