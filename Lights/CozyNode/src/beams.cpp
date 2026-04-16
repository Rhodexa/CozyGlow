#include "beams.h"
#include "config.h"
#include <Arduino.h>
#include <math.h>

// Pin and LEDC tables are taken directly from config — no local copy needed.

// ── State ─────────────────────────────────────────────────────────────────────
static uint8_t  s_frame[LED_CHANNEL_COUNT] = {};
static uint16_t s_lut[256]                 = {};
static bool     s_armed                    = false;
static float    s_ramp                     = 0.0f;  // 0.0 = dark, 1.0 = full output
static uint32_t s_last_tick_ms             = 0;

// ── Internal ──────────────────────────────────────────────────────────────────
static void build_gamma_lut() {
    s_lut[0] = 0;
    for (int i = 1; i < 256; i++) {
        s_lut[i] = (uint16_t)(powf(i / 255.0f, BEAM_GAMMA) * 1023.0f + 0.5f);
    }
}

static void flush_to_ledc() {
    for (uint8_t ch = 0; ch < LED_CHANNEL_COUNT; ch++) {
        ledcWrite(Ledc::LED_CH[ch], (uint32_t)(s_lut[s_frame[ch]] * s_ramp));
    }
}

// ── Public API ────────────────────────────────────────────────────────────────
void beams_init() {
    build_gamma_lut();

    for (uint8_t ch = 0; ch < LED_CHANNEL_COUNT; ch++) {
        ledcAttachChannel(Pin::LED[ch], Ledc::LED_FREQ_HZ, Ledc::LED_BITS, Ledc::LED_CH[ch]);
        ledcWrite(Ledc::LED_CH[ch], 0);
    }

    s_last_tick_ms = millis();
}

void beams_set(uint8_t channel, uint8_t value) {
    if (channel < LED_CHANNEL_COUNT) {
        s_frame[channel] = value;
    }
}

void beams_set_all(const uint8_t* values, uint8_t count) {
    uint8_t n = (count < LED_CHANNEL_COUNT) ? count : LED_CHANNEL_COUNT;
    for (uint8_t i = 0; i < n; i++) {
        s_frame[i] = values[i];
    }
}

void beams_arm() {
    s_armed = true;
}

void beams_disarm() {
    s_armed = false;
}

void beams_tick() {
    uint32_t now_ms = millis();
    float    dt_ms  = (float)(now_ms - s_last_tick_ms);
    s_last_tick_ms  = now_ms;

    if (s_armed && s_ramp < 1.0f) {
        s_ramp += dt_ms / (float)BEAM_RAMP_UP_MS;
        if (s_ramp >= 1.0f) s_ramp = 1.0f;
    } else if (!s_armed && s_ramp > 0.0f) {
        s_ramp -= dt_ms / (float)SIGNAL_LOSS_FADE_MS;
        if (s_ramp <= 0.0f) s_ramp = 0.0f;
    }

    flush_to_ledc();
}

bool beams_is_armed()       { return s_armed; }
bool beams_is_fully_armed() { return s_ramp >= 1.0f; }
bool beams_is_fully_off()   { return s_ramp <= 0.0f; }
