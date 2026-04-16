# CozyGlow Ecosystem — Architecture Notes
_Living document. Last meaningful update: 2026-04-01 (rev 2)_

---

## What This Is

A custom theatre/event lighting ecosystem built around:
- **Pegasi** — the lighting desk software (web-based, moving toward Electron)
- **Gabby** — the wireless lighting protocol (replaced older "Twili")
- **The Console** — a repurposed DMX controller becoming a MIDI control surface
- **High-CRI RGBWWCW lights** — 6-channel, individually addressable
- **LED tapes** — run procedural and pre-programmed animations on-cue
- **A MIDI keyboard** — quick access / manual animation triggering

---

## Communication Architecture

```
FL Studio  ──Virtual MIDI────────────────┐
Console    ──USB MIDI (+ USB CDC) ───────┤
Keyboard   ──USB MIDI────────────────────┴──► [Pegasi] ──USB Serial──► [Gabby Master] ──ESP-NOW──► [Lights]
                                               (hub)     (OSC/SLIP)      (wired USB)     (wireless)
```

### Key principle: Pegasi is the brain and router.
Nothing talks to anything directly except through Pegasi.
Input devices (console, keyboard, FL Studio) speak MIDI into Pegasi.
Pegasi → Master link is **wired USB Serial**, not wireless. No router in the path.
The only wireless hop in the system is **ESP-NOW** (Master → Lights).

### Power strategy: everything on 3.3V
All HC-series chips (74HC4051, HC245, HC573) are rated 2–6V. Running them at 3.3V means:
- HC outputs → RP2040 inputs: safe, no level shifters needed
- RP2040 outputs → HC inputs: valid HIGH at 3.3V VCC, no issues
- Fader wipers: swing 0–3.3V → straight into ADC, no voltage divider needed
Feed HC chips and fader resistive tracks from Pico 3.3V out (pin 36, 300mA max).
HC chips + faders draw ~15mA total — well within budget.
On the original PCB: cut/intercept the 5V rail feeding these chips, replace with 3.3V.

### Why NOT WiFi/UDP for Pegasi → Master:
- Adds router as a dependency (venues may not have reliable WiFi)
- Air latency + potential congestion
- If master's WiFi radio is busy with Pegasi comms, it competes with ESP-NOW on the same radio
- Wired USB serial: microsecond latency, zero network configuration, always works

---

## Protocol Decisions

### Console → Pegasi: MIDI (permanent)
- Faders → CC 14–31 on MIDI Channel 16
- Buttons with LEDs (Scan 1–12) → Note On/Off, bidirectional (Pegasi sends LED state back)
- Other buttons → Note On/Off on same channel
- Rationale: MIDI is right-shaped for control surface events. Low bandwidth, event-driven, universal.

### Pegasi → Gabby Master: OSC over USB Serial (SLIP framing)
- Wired. No WiFi. No router.
- OSC packets delimited using SLIP encoding over USB CDC serial port
- Full float resolution for color channels (no 7-bit limitation)
- Named address space for animation cues (see Animation System below)
- Supported by `osc.js` — works from browser via Web Serial API (Chrome), or natively in Electron
- Rationale: MIDI can't cleanly express animation cues with structured parameters. OSC can.
  Using serial instead of UDP keeps the master's WiFi radio free for pure ESP-NOW use.

### Gabby Master → Lights: ESP-NOW (keep forever)
- Uses 802.11 radio but bypasses the entire WiFi stack — no association, no DHCP, no TCP/IP
- Master + lights negotiate least-congested 2.4GHz channel (built-in handshake)
- No router dependency, no passwords, phones can't accidentally collide with it
- Don't touch this. It works.

### FL Studio → Lights (music sync):
- FL Studio → MIDI → Pegasi → OSC → Master → Lights
- FL Studio is just another MIDI input source for Pegasi
- Master doesn't need to know FL Studio exists
- This is a master firmware concern, not a Pegasi concern

---

## OSC Address Space (Draft)

```
/light/{id}/red         float 0.0–1.0
/light/{id}/green       float 0.0–1.0
/light/{id}/blue        float 0.0–1.0
/light/{id}/ww          float 0.0–1.0   warm white
/light/{id}/cw          float 0.0–1.0   cool white
/light/{id}/brightness  float 0.0–1.0   linear multiplier (gamma on device)
/light/{id}/rgbwwcw     float×6         all channels at once

/tape/{id}/anim         string name, ...args    trigger named animation
/tape/{id}/stop
```

Example animation cue:
```
/tape/1/anim  "wave"  start:5  speed:5.0
```

_Animation arg schema TBD per animation type._

---

## Animation System (LED Tapes)

- Tapes run animations **on-cue** — Pegasi fires a trigger, tape runs it autonomously
- Two types: **pre-programmed** (named, fixed) and **procedural** (parameterized filters)
- Parameters include things like: start LED position, speed, direction, color
- **Note pitch → LED position** mapping exists so the MIDI keyboard can manually
  trigger positional animations (e.g. play a chord, lights spread from those positions)
- MIDI bandwidth not a concern here because we're sending cues, not streaming frames

---

## MIDI CC Mapping (Gabby Master / Legacy)

The Gabby master uses an **inverted CC/channel mapping** (intentional):
- **CC number = light index** (0–127 → up to 128 lights)
- **MIDI channel = parameter within the light** (1–16 → 16 parameters)

Rationale: more fixtures than parameters in practice. Standard orientation
(channel=fixture, CC=parameter) would cap fixtures at 16.

This lives in master firmware config — swappable without touching the lights.

---

## The Console Hardware

### Physical
- 18 faders (sliders)
- 35 buttons
- 14 green LEDs (12 on scan buttons = bidirectional with Pegasi)
- 4-digit 7-segment display
- 18 WS2812 LEDs (one per fader — **may be removed, Pegasi has this info on screen**)

### Original circuitry being reused
- 74HC4051 × 3 — analog muxes for faders (8x3 matrix, single shared ADC line)
- HC245 (U3) — button reader
- HC573 (U5) — LED columns
- HC573 (U11) — MUX select (D0=S0, D1=S1, D2=S2, D3=IC0_/EN, D4=IC1_/EN, D5=IC2_/EN)
- HC573 (U4) — LED rows (drives transistors)
- HC573 (U16) — button scan rows (active LOW, no anti-ghosting — acceptable since user rarely presses 3+ buttons)
- U2 — original CPU's RAM chip, **completely useless, ignore**

### Original CPU socket pin map (key pins)
```
Pin 1  → U4 Latch Strobe  (LED rows)
Pin 2  → U5 Latch Strobe  (LED columns)
Pin 3  → U3 Button Read /EN
Pin 4  → U11 Latch Strobe (MUX select)
Pin 5  → Muxes Common (ADC line)
Pin 14 → U16 Latch Strobe (button scan)
Pins 33–40 → IO0–IO7 (8-bit data bus)
```

### Fader select bitmask (written to U11 via 8-bit bus)
```
Fader | D76543210    Notes
  0   | xx110000   ← IC0 active, MUX input 0
  1   | xx110001
  ...
  7   | xx110111   ← IC0 active, MUX input 7
  8   | xx101000   ← IC1 active, MUX input 0
  ...
 15   | xx101111   ← IC1 active, MUX input 7
 16   | xx011000   ← IC2 active, MUX input 0
 17   | xx011001   ← IC2 active, MUX input 1
```
D3=IC0_/EN, D4=IC1_/EN, D5=IC2_/EN (active LOW)
D0=S0, D1=S1, D2=S2

### RP2040 replacement plan
- Drop-in daughter board for original CPU socket (40-pin DIP footprint)
- 15 pins needed total (data bus × 8, strobe × 4, /EN × 1, ADC × 1, WS2812 × 1)
- USB Composite device: **USB MIDI + USB CDC simultaneously** (TinyUSB, no hardware switch)
- Logic level note: HC outputs are 5V, RP2040 GPIO is NOT 5V tolerant — add level shifters on input lines (HC→RP2040 direction). RP2040 3.3V outputs into HC inputs is fine (HC accepts >2.0V as HIGH).

### RP2040 pin mapping (decided, reflected in pins.h)
```
GPIO 0    → free (UART0 TX default — debug stdio during development)
GPIO 1    → free (UART0 RX default)
GPIO 2–9  → D0–D7 (8-bit data bus; outputs normally, briefly inputs during button read)
GPIO 10   → U4 Latch Strobe  (LED rows / 7-seg digits)
GPIO 11   → U5 Latch Strobe  (LED columns / 7-seg segments)
GPIO 12   → U11 Latch Strobe (MUX select + /EN lines)
GPIO 13   → U16 Latch Strobe (button scan rows)
GPIO 14   → U3 /EN           (button read enable, active LOW)
GPIO 15   → Mode switch      (pull-up; LOW = CDC mode, HIGH = USB MIDI mode)
GPIO 16   → UART0 TX         (MIDI out, 31250 baud — always active)
GPIO 17   → UART0 RX         (MIDI in, reserved for future)
GPIO 18   → WS2812 data      (PIO; leave unpopulated if LEDs removed)
GPIO 26   → ADC0             (mux common analog line)
GPIO 19–22, 27–29 → free
```

### Firmware behavior
- Faders: 12-bit ADC → scale to 7-bit CC with **hysteresis** (don't send update unless delta > N counts, prevents jitter spam)
- Buttons with LEDs: send Note On/Off, **also receive** Note On/Off from Pegasi to set LED state
- Display: show MIDI channel (Ch01–Ch16), selectable via button
- WS2812: use RP2040 PIO state machine (one pin, all 18 chained)

---

## Platform Notes

- **Pegasi is web-based** — intentional, allows live code editing during shows without recompilation
- **Moving to laptop** — primary platform going forward
- **Tablet/touchscreen** kept for color picking (RGBWWCW color wheel interaction)
- **Android only** — no iOS constraint to worry about
- **Electron is an option** if native protocol access (UDP for OSC, direct serial) becomes needed
- Web MIDI API works natively in Chrome/Edge — no bridge needed for MIDI input
- For OSC: browser can't do UDP directly → either Electron, or a lightweight local Node bridge

---

## Open Questions / Future Decisions

- [ ] WS2812s: keep or remove? (Current lean: remove, Pegasi has visual feedback)
- [ ] OSC arg schema for animations — needs to be defined per animation type
- [ ] Does the Gabby master need a firmware update to receive OSC? (Probably yes)
- [x] Level shifters: not needed — all HC chips powered from 3.3V (Pico pin 36)
- [ ] Custom PCB footprint for original CPU socket — needs measurement
- [ ] ADC hysteresis threshold — determine empirically once hardware is running
- [ ] Whether Pegasi stays pure web or moves to Electron — Web Serial API covers OSC/Serial in Chrome, so pure web is still viable
- [ ] Gabby master firmware: needs OSC/SLIP parser over USB serial (replaces or sits alongside existing MIDI handling)
