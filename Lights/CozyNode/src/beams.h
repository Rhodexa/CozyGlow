#pragma once
#include <cstdint>

// Beams engine — owns all LED output for the 6 light channels (R G B CW WW UV).
//
// Armed vs disarmed (target flag, set by state machine):
//   Armed    — fixture is assigned and has valid stream data; ramp climbs to 1.0.
//   Disarmed — signal lost or unassigned; ramp falls to 0.0.
//
// The ramp animates toward the target. You can query the target or the actual ramp state:
//   beams_is_armed()        — target flag (true even while still ramping up)
//   beams_is_fully_armed()  — ramp has reached 1.0
//   beams_is_fully_off()    — ramp has reached 0.0 (servo engine may cut power)
//
// Arming/disarming mid-ramp reverses direction smoothly — no jump.
//
// Fail-dark guarantee: ramp initialises at 0. No output until beams_arm() is called.

void beams_init();

// Write a single logical channel (0–5, use BEAM_CH_* constants). Out-of-range ignored.
void beams_set(uint8_t channel, uint8_t value);

// Write up to LED_CHANNEL_COUNT channels from a stream payload slice.
// Values beyond LED_CHANNEL_COUNT are ignored; uncovered channels keep their last value.
void beams_set_all(const uint8_t* values, uint8_t count);

// Set target to armed — begin ramping up. Idempotent if already fully armed.
void beams_arm();

// Set target to disarmed — begin ramping down. Idempotent if already fully off.
void beams_disarm();

// Advance the ramp and flush framebuffer × ramp to LEDC. Call once per main loop tick.
void beams_tick();

// Target state queries
bool beams_is_armed();         // true if target is armed (even mid-ramp)

// Ramp completion queries
bool beams_is_fully_armed();   // true when ramp == 1.0
bool beams_is_fully_off();     // true when ramp == 0.0 — servo engine may cut power
