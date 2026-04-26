#ifndef SYNTH_H
#define SYNTH_H

void synth_init(void);
void synth_load_demo_pattern(void);

// Call on sequencer step boundaries
void synth_on_step(unsigned char step_index);

// Call on ever audio ISR tick
signed int synth_tick_sample(void);

#endif