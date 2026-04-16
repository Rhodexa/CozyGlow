#include "diag_led.h"
#include "config.h"
#include <Arduino.h>
#include <FastLED.h>

// ── FastLED setup ─────────────────────────────────────────────────────────────
static CRGB s_led[1];

// ── Palette ───────────────────────────────────────────────────────────────────
// RGB values for each DiagColor state. Tune to match your WS2812 characteristics.
static const CRGB STATE_COLOURS[] = {
    /* Off          */ CRGB::Black,
    /* WhiteFlash   */ CRGB(255, 255, 255),
    /* RedBreathe   */ CRGB(255,  20,   0),
    /* GreenBreathe */ CRGB(  0,  80,   0),   // faint green per spec
    /* OrangeBreathe*/ CRGB(255,  80,   0),   // neon orange per spec
};

// ── Breathe animation ─────────────────────────────────────────────────────────
// Oscillates brightness between 50% and 100% using a sine wave.
// Returns a brightness multiplier [0.5, 1.0] based on elapsed time.
static float breathe_scale(uint32_t now_ms) {
    float phase = (float)(now_ms % 3000) / 3000.0f;   // 3-second breathe period
    return 0.5f + 0.5f * (0.5f + 0.5f * sinf(phase * 2.0f * (float)M_PI));
}

// ── State ─────────────────────────────────────────────────────────────────────
static DiagColor s_state            = DiagColor::Off;
static bool      s_override_active  = false;
static CRGB      s_override_colour  = CRGB::Black;
static uint8_t   s_override_t       = 0;     // remaining seconds (255 = indefinite)
static uint32_t  s_override_last_ms = 0;     // for counting down override seconds
static uint32_t  s_boot_flash_end   = 0;     // millis() when the boot flash should end

// ── Internal ──────────────────────────────────────────────────────────────────
static bool state_uses_breathe(DiagColor color) {
    return color == DiagColor::RedBreathe
        || color == DiagColor::GreenBreathe
        || color == DiagColor::OrangeBreathe;
}

static void apply_state_colour(uint32_t now_ms) {
    if (s_state == DiagColor::Off) {
        s_led[0] = CRGB::Black;
        return;
    }
    if (s_state == DiagColor::WhiteFlash) {
        s_led[0] = (now_ms < s_boot_flash_end) ? STATE_COLOURS[(int)DiagColor::WhiteFlash]
                                                : CRGB::Black;
        if (now_ms >= s_boot_flash_end) s_state = DiagColor::Off;
        return;
    }
    CRGB base = STATE_COLOURS[(int)s_state];
    float scale = state_uses_breathe(s_state) ? breathe_scale(now_ms) : 1.0f;
    s_led[0] = CRGB((uint8_t)(base.r * scale),
                    (uint8_t)(base.g * scale),
                    (uint8_t)(base.b * scale));
}

// ── Public API ────────────────────────────────────────────────────────────────
void diag_led_init() {
    FastLED.addLeds<WS2812, Pin::DIAG_LED, GRB>(s_led, 1);
    FastLED.setBrightness(255);
    s_led[0] = CRGB::Black;
    FastLED.show();

    // Boot self-test: white flash for 300ms — confirms all three WS2812 channels work
    // before any colour-coded status meaning applies.
    s_state         = DiagColor::WhiteFlash;
    s_boot_flash_end = millis() + 300;
}

void diag_led_set_state(DiagColor color) {
    if (color == DiagColor::WhiteFlash) return;   // WhiteFlash is boot-only
    s_state = color;
}

void diag_led_set_override(uint8_t r, uint8_t g, uint8_t b, uint8_t t) {
    if (t == 0) {
        diag_led_clear_override();
        return;
    }
    s_override_colour  = CRGB(r, g, b);
    s_override_t       = t;
    s_override_active  = true;
    s_override_last_ms = millis();
}

void diag_led_clear_override() {
    s_override_active = false;
    s_override_t      = 0;
}

void diag_led_tick() {
    uint32_t now_ms = millis();

    // Count down timed override (skip if indefinite = 255)
    if (s_override_active && s_override_t != 255) {
        if ((now_ms - s_override_last_ms) >= 1000) {
            s_override_last_ms += 1000;
            s_override_t--;
            if (s_override_t == 0) s_override_active = false;
        }
    }

    if (s_override_active) {
        s_led[0] = s_override_colour;
    } else {
        apply_state_colour(now_ms);
    }

    FastLED.show();
}
