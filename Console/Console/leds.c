#include "leds.h"
#include "bus.h"
#include "pins.h"
#include "pico/stdlib.h"

/*
    Segment layout (physical position on digit):
         3
        2 4
         6
        1 5
         0

    Font table — each byte encodes which segments are ON (bit N = segment N):
    digit  6 5 4 3 2 1 0
        0  [ 1 1 1 1 1 1 0 ]
        1  [ 0 0 0 0 1 1 0 ]
        2  [ 1 1 0 1 1 0 1 ]
        3  [ 1 0 0 1 1 1 1 ]
        4  [ 0 0 1 0 1 1 1 ]
        5  [ 1 0 1 1 0 1 1 ]
        6  [ 1 1 1 1 0 1 1 ]
        7  [ 0 0 0 1 1 1 0 ]
        8  [ 1 1 1 1 1 1 1 ]
        9  [ 1 0 1 1 1 1 1 ]
        some alphabetics
        A [0, 1, 1, 1, 1, 1, 1]
        b [1, 1, 1, 0, 0, 1, 1]
        c [1, 1, 0, 0, 0, 0, 1]
        d [1, 1, 0, 0, 1, 1, 1]
        e [1, 1, 1, 1, 0, 0, 1]
        f [0, 1, 1, 1, 0, 0, 1]
        G [1, 1, 1, 1, 0, 1, 0]
        H [0, 1, 1, 0, 1, 1, 1]
        h [0, 1, 1, 0, 0, 1, 1]
        I same as 1
        J [1, 0, 0, 0, 1, 1, 0]
        L [1, 1, 1, 0, 0, 0, 0]
        n [0, 1, 0, 0, 0, 1, 1]
        O same as 0
        o [1, 1, 0, 0, 0, 1, 1]
        P [0, 1, 1, 1, 1, 0, 1]
        Q [1, 1, 1, 1, 1, 0, 0]
        r [0, 1, 0, 0, 0, 0, 1]
        S same as 5
        t [1, 1, 1, 0, 0, 0, 1]
        u [1, 1, 0, 0, 0, 1, 0]
        Y same as 4
        ( [1, 1, 1, 1, 0, 0, 0]
        ) [1, 0, 0, 1, 1, 1, 0]
        - [0, 0, 0, 0, 0, 0, 1]
        ? [0, 1, 0, 1, 1, 0, 0]

    Row layout:
      Row 0  — indicators  0– 5  (cols 0–5)
      Row 1  — indicators  6–11  (cols 0–5), vertical LEDs 0–1 (cols 6–7)
      Row 2  — symbols (col 0–7):
                  0: upper dot / middle dash combo
                  1: lower dot (combined with 0 = ÷)
                  2: decimal point for 999.9
                  3: decimal point for 99.99   (3+7 = colon 99:99)
                  4: left dot tower — upper
                  5: left dot tower — middle
                  6: left dot tower — lower
                  7: apostrophe / colon half   (3+7 = colon 99:99)
      Row 3  — 1000s digit
      Row 4  —  100s digit
      Row 5  —   10s digit
      Row 6  —    1s digit

    PNP transistors on rows — active LOW on both axes.
    Hardware output: row_mask and col_mask both have 0 = active, 1 = idle.
*/

static const uint8_t font_digits[10] = {
    0x7E, // 0
    0x06, // 1
    0x6D, // 2
    0x4F, // 3
    0x17, // 4
    0x5B, // 5
    0x7B, // 6
    0x0E, // 7
    0x7F, // 8
    0x5F, // 9
};

// Returns segment byte for a printable character, 0x00 if unsupported (blank)
static uint8_t char_to_seg(char c) {
    switch (c) {
        case '0': case 'O': case 'o': return 0x7E;
        case '1': case 'I': case 'i': return 0x06;
        case '2':            return 0x6D;
        case '3':            return 0x4F;
        case '4': case 'Y': case 'y': return 0x17;
        case '5': case 'S': case 's': return 0x5B;
        case '6':            return 0x7B;
        case '7':            return 0x0E;
        case '8':            return 0x7F;
        case '9':            return 0x5F;
        case 'A': case 'a': return 0x3F;
        case 'b':            return 0x73;
        case 'c':            return 0x61;
        case 'd':            return 0x67;
        case 'E': case 'e': return 0x79;
        case 'F': case 'f': return 0x39;
        case 'G':            return 0x7A;
        case 'H':            return 0x37;
        case 'h':            return 0x33;
        case 'J': case 'j': return 0x46;
        case 'L': case 'l': return 0x70;
        case 'n':            return 0x23;
        case 'P': case 'p': return 0x3D;
        case 'Q': case 'q': return 0x7C;
        case 'r':            return 0x21;
        case 't':            return 0x71;
        case 'u': case 'U': return 0x62;
        case '(':            return 0x78;
        case ')':            return 0x4E;
        case '-':            return 0x01;
        case '?':            return 0x2C;
        case ' ':            return 0x00;
        default:             return 0x00;
    }
}

static uint8_t  digits[4]     = {0, 0, 0, 0}; // raw segment bytes for each digit position
static uint8_t  symbols       = 0x00;
static uint16_t indicators    = 0x0000;        // bits 0–13
static uint8_t  current_row   = 0;

static uint8_t col_data_for_row(uint8_t row) {
    switch (row) {
        case 0: return (uint8_t)(indicators & 0x3F);
        case 1: return (uint8_t)((indicators >> 6) & 0xFF);
        case 2: return symbols;
        case 3: return digits[0];
        case 4: return digits[1];
        case 5: return digits[2];
        case 6: return digits[3];
        default: return 0x00;
    }
}

void leds_init(void) {
    latch_write(0xFF, PIN_STROBE_LED_ROWS);
    latch_write(0xFF, PIN_STROBE_LED_COLS);
}

void leds_tick(void) {
    uint8_t col = col_data_for_row(current_row);
    latch_write(~(1u << current_row), PIN_STROBE_LED_ROWS);
    latch_write(~col,                 PIN_STROBE_LED_COLS);
    current_row = (current_row + 1) % 7;
}

void display_str(const char *s) {
    for (uint8_t i = 0; i < 4; i++)
        digits[i] = s[i] ? char_to_seg(s[i]) : 0x00;
}

void display_number(uint16_t n) {
    if (n > 9999) n = 9999;
    digits[0] = font_digits[n / 1000];
    digits[1] = font_digits[(n / 100) % 10];
    digits[2] = font_digits[(n / 10)  % 10];
    digits[3] = font_digits[n % 10];
}

void display_symbols(uint8_t mask) {
    symbols = mask;
}

void indicator_set(uint8_t idx, bool on) {
    if (idx >= 14) return;
    if (on) indicators |=  (1u << idx);
    else    indicators &= ~(1u << idx);
}
