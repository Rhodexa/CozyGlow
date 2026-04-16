#include "buttons.h"
#include "bus.h"
#include "pins.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

#define ROW_COUNT 6
#define COL_COUNT 6

static uint8_t prev[ROW_COUNT];

void buttons_init(void) {
    gpio_init(PIN_BTN_READ_EN);
    gpio_set_dir(PIN_BTN_READ_EN, GPIO_OUT);
    gpio_put(PIN_BTN_READ_EN, 1);
    memset(prev, 0xFF, sizeof(prev));
    latch_write(0xFF, PIN_STROBE_BTN_SCAN);
}

void buttons_scan(void) {
    gpio_set_dir_in_masked(BUS_PIN_MASK);

    uint8_t state[ROW_COUNT];
    for (uint8_t row = 0; row < ROW_COUNT; row++) {
        latch_write(~(1u << row), PIN_STROBE_BTN_SCAN);
        gpio_put(PIN_BTN_READ_EN, 0);
        __asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
        state[row] = (uint8_t)((gpio_get_all() >> BUS_PIN_BASE) & 0xFF);
        gpio_put(PIN_BTN_READ_EN, 1);
    }

    latch_write(0xFF, PIN_STROBE_BTN_SCAN);
    gpio_set_dir_out_masked(BUS_PIN_MASK);

    for (uint8_t row = 0; row < ROW_COUNT; row++) {
        uint8_t changed = state[row] ^ prev[row];
        for (uint8_t col = 0; col < COL_COUNT; col++) {
            if (!(changed & (1u << col))) continue;
            bool pressed = !(state[row] & (1u << col));
            printf("BTN R%d C%d %s\n", row, col, pressed ? "DOWN" : "UP");
        }
        prev[row] = state[row];
    }
}
