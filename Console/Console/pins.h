#pragma once

// ---------------------------------------------------------------------------
// 8-bit data bus — consecutive GPIOs for atomic masked read/write
// Starts at GPIO 2, leaving GPIO 0/1 free as the default UART0 TX/RX pins
// (handy for debug printf without any rewiring during development).
// ---------------------------------------------------------------------------
#define PIN_D0          2
#define PIN_D7          9
#define BUS_PIN_BASE    PIN_D0
#define BUS_PIN_MASK    (0xFFu << BUS_PIN_BASE)

// ---------------------------------------------------------------------------
// Latch strobes — pulse HIGH to latch current bus value into the HC573
// ---------------------------------------------------------------------------
#define PIN_STROBE_LED_ROWS  10  // U4  — drives LED row transistors + 7-seg digits
#define PIN_STROBE_LED_COLS  11  // U5  — drives LED columns + 7-seg segments
#define PIN_STROBE_MUX_SEL   13  // U11 — fader mux select (S0/S1/S2) + chip /EN lines
#define PIN_STROBE_BTN_SCAN  14  // U16 — button matrix scan rows (active LOW output)

// ---------------------------------------------------------------------------
// Button read enable
// U3 is an HC245 bus transceiver. Assert LOW to place button column state
// onto the data bus (requires GPIO 2-9 switched to INPUT at that moment).
// ---------------------------------------------------------------------------
#define PIN_BTN_READ_EN  12  // U3 /EN — active LOW

// ---------------------------------------------------------------------------
// Boot-time mode switch
// Internal pull-up enabled. Read once at startup before USB init.
//   HIGH (default / floating) = USB MIDI mode
//   LOW  (switch closed)      = USB CDC mode (MIDI bytes over serial)
// ---------------------------------------------------------------------------
#define PIN_MODE_SWITCH  15

// ---------------------------------------------------------------------------
// WS2812 LED strip — driven by PIO state machine
// One data line for all 18 chained LEDs (one per fader)
// ---------------------------------------------------------------------------
#define PIN_WS2812  16

// ---------------------------------------------------------------------------
// UART MIDI — always active regardless of USB mode
// UART0 default TX/RX pins — simplest board layout, no conflict since
// pico_enable_stdio_uart is disabled.
// Standard MIDI baud: 31250
// ---------------------------------------------------------------------------
#define PIN_MIDI_TX  0  // UART0 TX — MIDI output
#define PIN_MIDI_RX  1  // UART0 RX — MIDI input (reserved, not used yet)
#define MIDI_UART    uart0
#define MIDI_BAUD    31250

// ---------------------------------------------------------------------------
// ADC — single shared analog bus from all three 74HC4051 mux chips
// All mux /EN lines are managed via U11; only one chip enabled at a time.
// ---------------------------------------------------------------------------
#define PIN_ADC_MUX   26  // ADC0
#define ADC_INPUT_NUM  0  // adc_select_input() argument for GPIO26
