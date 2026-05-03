# Controls Summary

This file documents the control behavior currently implemented in `src/main.c`.

## Screen Modes

- `EDIT` mode:
  - Step editing/navigation controls are active.
  - Top-row selected step blinks using a filled block (`UI_CURSOR_BLOCK_CHAR`).
- `PLAY` mode:
  - Edit controls are disabled.
  - Top-row playhead shows `*` at the current sequencer step.

Mode toggle is on `RB3` (edge-detected, debounced), and works in both modes.

## Track Pages

- `RB0` cycles track page in `EDIT` mode:
  - `K` (kick) -> `S` (snare) -> `H` (hihat) -> `Y` (synth) -> `K`

Top row header shows page + 8-step window, for example `K1:` then 8 slots.

## Cursor Navigation (EDIT mode only)

- `RB1`: move edit cursor left
- `RB2`: move edit cursor right

## Step Editing (EDIT mode only)

- `RB5`: toggle selected step value on the active page
  - `K/S/H` pages: direct `OFF <-> ON` step toggle
  - `Y` page: direct synth step `OFF <-> ON` toggle
    - `ON` writes the currently selected synth note index

## BPM Editing

- `RB4` (EDIT mode only): toggle BPM edit arm/disarm
- When BPM edit is armed:
  - BPM value blinks in row 2
  - `RB6`: BPM down (fast debounce)
  - `RB7`: BPM up (fast debounce)

Configured bounds and step are defined in `include/project_config.h`:
- `BPM_MIN`
- `BPM_MAX`
- `BPM_STEP`

## Synth Step Editing (EDIT mode, `Y` page, BPM edit OFF)

- `RB6`: cycle selected synth step letter `A..G` with wrap
  - applies only if the selected synth step is already `ON`
  - if selected synth step is `OFF`, no letter edit is applied
- `RB7`: cycle selected synth step octave `3..5` with wrap
  - applies only if the selected synth step is already `ON`
  - updates status row `O:x` to match the edited octave

When not in the synth-edit context above:
- `RB6` and `RB7` keep BPM down/up behavior (while in `EDIT` mode).

## Status Row (Row 2)

- `BPM` shown at cols `4..6`
- `O:x` shown at cols `8..10` only on synth page (`Y`)
- mode label at cols `13..15`: `EDT` or `PLY`

## Hardware Input Configuration

Current control mapping expects all `PORTB` control buttons as inputs:
- `TRISB = 0xFF`

This supports `RB0..RB7` button reads in the current implementation.

## Completion Note

Controls documentation is updated to match the current implemented behavior.
