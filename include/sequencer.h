#ifndef SEQUENCER_H
#define SEQUENCER_H

void seq_init(void);
void seq_tick(void);

void seq_set_bpm(unsigned int bpm);
void seq_set_pattern_steps(unsigned char steps);
void seq_set_steps_per_quarter(unsigned char spq);

unsigned int seq_get_bpm(void);
unsigned char seq_get_step(void);
unsigned char seq_get_pattern_steps(void);
unsigned char seq_get_steps_per_quarter(void);

unsigned char seq_step_advanced(void);

#endif