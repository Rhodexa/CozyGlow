#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

#include "pins.h"
#include "faders.h"
#include "buttons.h"
#include "leds.h"

static void bus_init(void) {
    for (uint32_t pin = PIN_D0; pin <= PIN_D7; pin++) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_pull_up(pin);
    }
    const uint32_t strobes[] = {
        PIN_STROBE_LED_ROWS, PIN_STROBE_LED_COLS,
        PIN_STROBE_MUX_SEL,  PIN_STROBE_BTN_SCAN,
    };
    for (size_t i = 0; i < sizeof(strobes) / sizeof(strobes[0]); i++) {
        gpio_init(strobes[i]);
        gpio_set_dir(strobes[i], GPIO_OUT);
        gpio_put(strobes[i], 0);
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(1500); // let USB serial settle before printing

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    bus_init();
    leds_init();
    faders_init();
    buttons_init();

    printf("=== CozyGlow Console - test mode ===\n\n");

    display_number(0);

    while (true) {
        leds_tick();
        faders_scan();
        buttons_scan();

        uint8_t idx, cc;
        if (faders_last_changed(&idx, &cc)) {
            display_number(cc);
            uint8_t bars = (uint8_t)((cc * 12 + 63) / 127);
            for (uint8_t i = 0; i < 12; i++) indicator_set(i, i < bars);
        }
    }
}
