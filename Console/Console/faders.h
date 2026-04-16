#pragma once
#include <stdint.h>
#include <stdbool.h>

void  faders_init(void);
void  faders_scan(void);
bool  faders_last_changed(uint8_t *out_idx, uint8_t *out_cc);
