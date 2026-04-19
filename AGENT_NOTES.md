# Drum Machine Project Notes

## Project
- Target: PIC18F4620 @ 40 MHz
- Outputs:
  - CCP1 (RC2): Drum channel (kick + snare + hihat mixed in software)
  - CCP2 (RC1): Synth channel
- Final mix: RC1 + RC2 through 4.7k resistors into audio out

## Requirements (Current)
- Sequencer length options: 8 / 16 / 24 / 32 bars
- Instruments:
  - Kick
  - Snare
  - Hi-hat
  - 1 synth voice (plain waveform)
- Adjustable BPM
- Variable BPM tempo (runtime adjustable, sequencer step interval recalculated from BPM)
- LCD 16x2:
  - Show 8 step slots at a time
  - Drum edit: filled cell for active step
  - Synth edit: show note text in slot

## Timing Plan
- Fast ISR: audio sample update (~20-40 kHz)
- Slow tick: sequencer step advance from BPM
- Keep ISR short and fixed-point only

## Data Model (Planned)
- Drum patterns as bitmasks (or arrays if >16 steps windowed)
- Synth notes as per-step note indices
- Runtime voice state:
  - Drum envelopes for kick/snare/hihat
  - Synth phase accumulator + frequency increment

## Next Inputs From User
- Note frequency mapping code
- LCD pin map and current LCD driver
- Input controls (buttons/encoder) pin map

## Current Repo Status
- Imported `lcd_portd.h` and `lcd_portd.c`
- Added `main.c` scaffold:
  - Timer0 ISR for audio sample clock
  - CCP1/CCP2 PWM output path for drum/synth
  - Basic drum voice synthesis placeholders
  - Sequencer step timing from BPM
  - No keypad dependencies

## Implementation Order
1. Define clock/timer constants and global timing model
2. Build audio ISR + PWM output path
3. Add drum voice synthesis and software drum mix
4. Add synth oscillator + note-frequency table integration
5. Add sequencer engine (BPM + step advance + pattern trigger)
6. Add LCD rendering for 8-slot window and edit modes
7. Add input handling and mode/state transitions
8. Tune levels/decay and verify timing stability
