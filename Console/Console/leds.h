#pragma once
#include <stdint.h>
#include <stdbool.h>

void leds_init(void);
void leds_tick(void);              // call every loop iteration — drives multiplexing

void display_number(uint16_t n);   // 0–9999 on the 4-digit display
void display_str(const char *s);   // up to 4 chars, e.g. "FAdr", "bUSY", "----"
void display_symbols(uint8_t mask);// raw control of row-2 symbols (see leds.c for bit map)
void indicator_set(uint8_t idx, bool on); // indicators 0–13
