#pragma once
#include "hardware/gpio.h"
#include "pins.h"

static inline void bus_write(uint8_t value) {
    gpio_put_masked(BUS_PIN_MASK, (uint32_t)value << BUS_PIN_BASE);
}

static inline void latch_strobe(uint32_t pin) {
    gpio_put(pin, 1);
    __asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
    gpio_put(pin, 0);
}

static inline void latch_write(uint8_t value, uint32_t strobe_pin) {
    bus_write(value);
    latch_strobe(strobe_pin);
}
