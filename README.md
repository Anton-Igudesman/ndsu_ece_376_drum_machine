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
- Main bring-up file created:
  - `src/main.c`

## Current Architecture (In Progress)
- Timer0 interrupt planned as fixed audio tick (`~25 kHz`)
- CCP PWM channels used as two audio outputs
- Duty-cycle update functions used to map audio sample level to PWM registers
- LCD shows initialization/bring-up status

## Repository Layout
```text
term_project/
  include/
    lcd_portd.h
    lcd_helpers.h
  src/
    lcd_portd.c
    lcd_helpers.c
    main.c
  AGENT_NOTES.md
  README.md
```

## Next Milestones
1. Finalize stable hardware bring-up (`LCD + PWM + timer ISR`).
2. Add sequencer timing from BPM (step-advance engine).
3. Add drum voice synthesis and per-step triggering.
4. Add synth oscillator and note-frequency mapping.
5. Add LCD edit modes for drums/synth and slot windowing.

## Toolchain Note
This project currently uses legacy-style includes (for example `#include <pic18.h>`) from:
`C:/Program Files (x86)/HI-TECH Software/PICC-18/PRO/9.63/include`
