# Drum Machine Controls (Quick Guide)

Use this as the button cheat sheet while running the project.

## 1) Screen Modes

- `RB3`: switch between `EDT` (edit) and `PLY` (play).
- Entering `EDT` always stops playback.

## 2) PLAY Mode (`PLY`)

- `RB4`: start/stop playback.
- No step editing in this mode.

## 3) EDIT Mode (`EDT`)

- `RB0`: change track page (`K` -> `S` -> `H` -> `Y` -> repeat).
- `RB1`: move cursor left.
- `RB2`: move cursor right.
- `RB5`: toggle selected step ON/OFF.

### BPM edit in EDIT mode

- `RB4`: toggle BPM edit ON/OFF.
- While BPM edit is ON:
  - `RB6`: BPM down.
  - `RB7`: BPM up.

## 4) Synth Editing (`Y` page in EDIT mode)

- First, select a step with `RB1/RB2`.
- Press `RB5` to turn that synth step ON.
- With that step ON:
  - `RB6`: cycle note letter (`A`..`G`, wraps).
  - `RB7`: cycle octave (`3`..`5`, wraps).
- If step is OFF, `RB6/RB7` do not change note/octave.

## 5) What You See on LCD

- Top row: current page (`K/S/H/Y`), window number, and 8 step slots.
- In `EDT`: selected step blinks as a block.
- In `PLY`: playhead shows `*`.
- Bottom row: BPM at left, `O:x` on synth page, mode label at right (`EDT` or `PLY`).
