#ifndef SYNTH_H
#define SYNTH_H

void synth_init(void);
void synth_load_demo_pattern(void);

// Call on sequencer step boundaries
void synth_on_step(unsigned char step_index);

// Call on ever audio ISR tick
signed int synth_tick_sample(void);

// Read note index stored at a sequencer step
unsigned char synth_get_note_at_step(unsigned char step_index);

// Write note index (0=OFF, 1..21=A3..G5) into a synth step
void synth_set_note_at_step(unsigned char step_index, unsigned char note_index);

#endif