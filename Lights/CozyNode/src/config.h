#pragma once
#include <cstdint>

// ── Firmware Identity ─────────────────────────────────────────────────────────
constexpr uint8_t FIRMWARE_VERSION = 1;

// ── Protocol Identity ─────────────────────────────────────────────────────────
constexpr uint8_t MAGIC_WORD[3]      = { 0x43, 0x47, 0x4E };  // "CGN" — CozyGlow Node
constexpr uint8_t ESPNOW_CHANNELS[3] = { 1, 6, 11 };           // Standard non-overlapping WiFi channels

// ── Timing ────────────────────────────────────────────────────────────────────
constexpr uint32_t CHANNEL_SCAN_DWELL_MS  = 1500;  // Listen on each channel before advancing in scan
constexpr uint32_t SIGNAL_LOSS_TIMEOUT_MS = 1500;  // No stream for this long → enter scan
constexpr uint32_t SIGNAL_LOSS_FADE_MS    = 2000;  // Beams ramp to zero over this duration on signal loss

// ── Beams Engine ─────────────────────────────────────────────────────────────
constexpr float    BEAM_GAMMA      = 2.2f;  // Perceptual gamma — tune empirically
constexpr uint32_t BEAM_RAMP_UP_MS = 500;   // Restore-from-black on stream resume

// ── Output Channel Counts (hardware facts) ────────────────────────────────────
// These describe how many PWM outputs exist on this board.
// What each channel carries is determined by the master's slot assignment — not this firmware.
constexpr uint8_t LED_CHANNEL_COUNT   = 6;
constexpr uint8_t SERVO_CHANNEL_COUNT = 2;

// ── GPIO Pin Assignments ──────────────────────────────────────────────────────
namespace Pin {
    // ADC inputs — ADC1 only, WiFi-safe
    constexpr uint8_t BATTERY_ADC = 36;   // ADC1_CH0, input-only
    constexpr uint8_t TEMP_ADC    = 39;   // ADC1_CH3, input-only

    // LED PWM outputs — LEDC Timer A (40 kHz, 10-bit)
    // Indices 0–5 map to the 6 output pads in physical board order.
    // The master decides what each index carries (R, G, B, CW, WW, UV, or anything else).
    constexpr uint8_t LED[LED_CHANNEL_COUNT] = { 32, 33, 25, 26, 27, 13 };

    // Servo PWM outputs — LEDC Timer B (50 Hz, 16-bit)
    constexpr uint8_t SERVO[SERVO_CHANNEL_COUNT] = { 16, 17 };

    // Diagnostic status LED (WS2812 — timing-sensitive, isolated)
    constexpr uint8_t DIAG_LED = 19;

    // Expansion headers — I2C and SPI, reserved for future use
    constexpr uint8_t I2C_SDA  = 21;
    constexpr uint8_t I2C_SCL  = 22;
    constexpr uint8_t SPI_MOSI = 23;
}

// ── LEDC Configuration ────────────────────────────────────────────────────────
namespace Ledc {
    // LED channels — 40 kHz, camera-safe, audio rate, 10-bit resolution
    constexpr uint32_t LED_FREQ_HZ = 40000;
    constexpr uint8_t  LED_BITS    = 10;

    // Servo channels — 50 Hz standard RC PWM, 16-bit for pulse precision
    constexpr uint32_t SERVO_FREQ_HZ = 50;
    constexpr uint8_t  SERVO_BITS    = 16;

    // Hardware timer assignments
    constexpr uint8_t TIMER_LED   = 0;   // Timer A — all LED channels
    constexpr uint8_t TIMER_SERVO = 1;   // Timer B — both servo channels

    // Hardware LEDC channel assignments — parallel to Pin::LED and Pin::SERVO
    constexpr uint8_t LED_CH[LED_CHANNEL_COUNT]     = { 0, 1, 2, 3, 4, 5 };
    constexpr uint8_t SERVO_CH[SERVO_CHANNEL_COUNT] = { 6, 7 };
}
