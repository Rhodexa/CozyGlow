#pragma once
#include <cstdint>

// ── Firmware Identity ─────────────────────────────────────────────────────────
constexpr uint8_t FIRMWARE_VERSION = 1;

// ── Protocol Identity ─────────────────────────────────────────────────────────
constexpr uint8_t  MAGIC_WORD[3]      = { 0x43, 0x47, 0x4E };  // "CGN" — CozyGlow Node
constexpr uint8_t  ESPNOW_CHANNELS[3] = { 1, 6, 11 };           // Standard non-overlapping WiFi channels
constexpr uint8_t  IMPOSSIBLE_INDEX   = 0xFF;                    // Sentinel: node not yet assigned a slot

// ── Timing ────────────────────────────────────────────────────────────────────
constexpr uint32_t CHANNEL_SCAN_DWELL_MS  = 1500;  // Listen on each channel before advancing in scan
constexpr uint32_t SIGNAL_LOSS_TIMEOUT_MS = 1500;  // No stream for this long → signal lost
constexpr uint32_t SIGNAL_LOSS_FADE_MS    = 2000;  // Beams ramp to zero over this duration on signal loss

// ── Beams Engine ─────────────────────────────────────────────────────────────
constexpr float    BEAM_GAMMA       = 2.2f;   // Perceptual gamma — tune empirically
constexpr uint32_t BEAM_RAMP_UP_MS  = 500;    // Restore-from-black on stream resume
// Ramp-down duration reuses SIGNAL_LOSS_FADE_MS

// ── Beam Layout ───────────────────────────────────────────────────────────────
// Logical channel indices within a fixture's payload slice
// Unused trailing channels are simply ignored — no special handling needed
constexpr uint8_t BEAM_CH_R    = 0;
constexpr uint8_t BEAM_CH_G    = 1;
constexpr uint8_t BEAM_CH_B    = 2;
constexpr uint8_t BEAM_CH_CW   = 3;
constexpr uint8_t BEAM_CH_WW   = 4;
constexpr uint8_t BEAM_CH_UV   = 5;
constexpr uint8_t BEAM_CH_PAN  = 6;
constexpr uint8_t BEAM_CH_TILT = 7;

constexpr uint8_t LED_CHANNEL_COUNT   = 6;   // R G B CW WW UV — driven by beams engine
constexpr uint8_t SERVO_CHANNEL_COUNT = 2;   // Pan Tilt       — driven by servo engine
constexpr uint8_t MAX_SPAN            = LED_CHANNEL_COUNT + SERVO_CHANNEL_COUNT;

// ── GPIO Pin Assignments ──────────────────────────────────────────────────────
namespace Pin {
    // ADC inputs — ADC1 only, WiFi-safe
    constexpr uint8_t BATTERY_ADC = 36;   // ADC1_CH0, input-only
    constexpr uint8_t TEMP_ADC    = 39;   // ADC1_CH3, input-only

    // LED PWM — LEDC Timer A (40 kHz, 10-bit)
    constexpr uint8_t LED_R  = 32;
    constexpr uint8_t LED_G  = 33;
    constexpr uint8_t LED_B  = 25;
    constexpr uint8_t LED_CW = 26;
    constexpr uint8_t LED_WW = 27;
    constexpr uint8_t LED_UV = 13;

    // Servo PWM — LEDC Timer B (50 Hz, 16-bit)
    constexpr uint8_t SERVO_PAN  = 16;
    constexpr uint8_t SERVO_TILT = 17;

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

    // Hardware channel assignments (LEDC channels, distinct from beam logical channels)
    constexpr uint8_t CH_LED_R    = 0;
    constexpr uint8_t CH_LED_G    = 1;
    constexpr uint8_t CH_LED_B    = 2;
    constexpr uint8_t CH_LED_CW   = 3;
    constexpr uint8_t CH_LED_WW   = 4;
    constexpr uint8_t CH_LED_UV   = 5;
    constexpr uint8_t CH_SERVO_PAN  = 6;
    constexpr uint8_t CH_SERVO_TILT = 7;
}
