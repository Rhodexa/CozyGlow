#include "faders.h"
#include "bus.h"
#include "pins.h"
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#define FADER_COUNT 18

static const uint8_t mux_sel[FADER_COUNT] = {
    0b00110000, 0b00110001, 0b00110010, 0b00110011,
    0b00110100, 0b00110101, 0b00110110, 0b00110111,
    0b00101000, 0b00101001, 0b00101010, 0b00101011,
    0b00101100, 0b00101101, 0b00101110, 0b00101111,
    0b00011000, 0b00011001,
};

static uint8_t last_cc[FADER_COUNT];
static uint8_t changed_idx = 0xFF;
static uint8_t changed_cc  = 0;

static uint8_t read_cc(uint8_t idx) {
    latch_write(mux_sel[idx], PIN_STROBE_MUX_SEL);
    sleep_us(10);
    return (uint8_t)(adc_read() >> 5);
}

void faders_init(void) {
    adc_init();
    adc_gpio_init(PIN_ADC_MUX);
    adc_select_input(ADC_INPUT_NUM);
    memset(last_cc, 0xFF, sizeof(last_cc));
}

void faders_scan(void) {
    static absolute_time_t next_dump;
    bool any_change = false;
    uint8_t cc[FADER_COUNT];

    for (uint8_t i = 0; i < FADER_COUNT; i++) {
        cc[i] = read_cc(i);
        if (cc[i] != last_cc[i]) {
            last_cc[i]  = cc[i];
            changed_idx = i;
            changed_cc  = cc[i];
            any_change  = true;
        }
    }

    if (!any_change && absolute_time_diff_us(get_absolute_time(), next_dump) > 0) return;
    next_dump = make_timeout_time_ms(500);

    // temp disable to debug buttons
    //printf("FAD:");
    //for (uint8_t i = 0; i < FADER_COUNT; i++) printf(" %3d", cc[i]);
    //printf("\n");
}

bool faders_last_changed(uint8_t *out_idx, uint8_t *out_cc) {
    if (changed_idx == 0xFF) return false;
    *out_idx    = changed_idx;
    *out_cc     = changed_cc;
    changed_idx = 0xFF;
    return true;
}
