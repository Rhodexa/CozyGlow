#pragma once
#include <cstdint>

// Diagnostic LED engine — drives the single WS2812 status indicator (Pin::DIAG_LED).
//
// Two layers of control, in priority order:
//   1. Operator override — raw RGB + duration, set via config packet.
//      Active while diag_override_t > 0 (or indefinitely if 255).
//      Cleared automatically when the duration expires, or explicitly via diag_led_clear_override().
//   2. Node state colour — one of the DiagColor palette entries, set by the state machine.
//      Active whenever no override is in effect.
//
// diag_led_tick() must be called once per main loop tick to advance animations and
// count down the override timer.

// Palette of named state colours used by the node state machine.
// Each entry maps to a specific RGB colour defined in diag_led.cpp.
enum class DiagColor {
    Off,           // Show running — diag LED dark (camouflaged)
    WhiteFlash,    // Boot self-test — brief white flash confirming all three WS2812 channels work
    RedBreathe,    // Searching — no signal on any channel
    GreenBreathe,  // Locked — stream heard, not yet assigned
    OrangeBreathe, // Signal lost — had assignment, lost stream
};

void diag_led_init();

// Set the current node state colour. Active when no override is in effect.
void diag_led_set_state(DiagColor color);

// Set a raw RGB override from a config packet.
// t: 0 = cancel any active override, 1–254 = seconds, 255 = indefinite.
void diag_led_set_override(uint8_t r, uint8_t g, uint8_t b, uint8_t t);

// Cancel any active override, reverting to the current state colour immediately.
void diag_led_clear_override();

// Advance animations and count down the override timer. Call once per main loop tick.
void diag_led_tick();
