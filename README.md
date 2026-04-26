# ECE 376 Term Project

PIC18F4620 drum machine + synth project with LCD UI and timer/interrupt-driven audio output.

## Target Platform
- MCU: `PIC18F4620`
- Clock: `40 MHz` oscillator (`Fcy = 10 MHz`)
- Display: `16x2` character LCD
- Audio outputs:
  - `RC2 / CCP1` = drum channel
  - `RC1 / CCP2` = synth channel
- Final analog mix: passive resistor mixer (recommended `4.7k` per channel)

## Project Goals
- Programmable sequence lengths based on bars: `8 / 16 / 24 / 32`
- Drum voices:
  - Kick
  - Snare
  - Hi-hat
- One synth voice (plain waveform)
- Runtime-adjustable BPM
- LCD step editing workflow:
  - 8 slots shown at a time
  - Drum steps shown as filled/active cells
  - Synth steps show note text

## Current Code Status
- LCD driver imported and organized:
  - `src/lcd_portd.c`
  - `include/lcd_portd.h`
- LCD helper module added:
  - `src/lcd_helpers.c`
  - `include/lcd_helpers.h`
- Shared config and constants added:
  - `include/project_config.h`
- Sequencer module added:
  - `include/sequencer.h`
  - `src/sequencer.c`
- Synth module added:
  - `include/synth.h`
  - `src/synth.c`
- Main integration file:
  - `src/main.c` (drum engine + ISR + LCD glue, ongoing modular split)

## Current Architecture (In Progress)
- Timer0 interrupt used as fixed audio tick (`~25 kHz`)
- Sequencer step timing derived from BPM/steps-per-quarter in dedicated module
- CCP PWM channels used as two audio outputs:
  - CCP1/RC2 drum channel
  - CCP2/RC1 synth channel
- Drum synthesis and synth synthesis both active in current code path
- LCD runtime update logic is being refined for reliable on-the-fly UI updates

## Hardware Validation Log
- Date: `2026-04-19`
- Test: PWM baseline verification on oscilloscope
- Probe points:
  - `RC2 / CCP1`
  - `RC1 / CCP2`
- Scope observation:
  - Stable repeating PWM waveform
  - Approximately just under 4 cycles per `50 us` division
  - Estimated PWM carrier frequency `~78 kHz`
- Expected value from configuration (`Fosc=40 MHz`, `PR2=0x7F`, `T2 prescale=1`):
  - `Fpwm = 40e6 / (4 * (127+1) * 1) = 78.125 kHz`
- Result: PASS (observed waveform matches expected PWM frequency)

## Repository Layout
```text
term_project/
  include/
    project_config.h
    sequencer.h
    synth.h
    lcd_portd.h
    lcd_helpers.h
  src/
    sequencer.c
    synth.c
    lcd_portd.c
    lcd_helpers.c
    main.c
  AGENT_NOTES.md
  README.md
```

## Next Milestones
1. Finish clean `main.c` integration to exclusively use sequencer module API.
2. Stabilize runtime LCD update path (event-gated updates while ISR audio runs).
3. Expand/edit pattern content for 32-step demonstrations when enabled.
4. Add mode-based LCD edit screens (drum/synth/config) with 8-step window paging.
5. Perform dedicated sound-design tuning pass (drum/synth balance and envelope shaping).

## Toolchain Note
This project currently uses legacy-style includes (for example `#include <pic18.h>`) from:
`C:/Program Files (x86)/HI-TECH Software/PICC-18/PRO/9.63/include`
