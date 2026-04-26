#include <pic18.h>
#include "project_config.h"
#include "sequencer.h"

// Internal sequencer state
static volatile unsigned int s_bpm = BPM_DEFAULT;
static volatile unsigned char s_step = 0;
static volatile unsigned char s_pattern_steps = PATTERN_STEPS_DEFAULT; // 8/16/24/32
static volatile unsigned char s_steps_per_quarter = STEPS_PER_QUARTER_DEFAULT; // timing resolution

static volatile unsigned long s_samples_per_step = 0;
static volatile unsigned long s_step_accum = 0;
static volatile unsigned char s_step_advanced_flag = 0;

/*
   Compute ISR-ticks-per-step from musical settings

   steps_per_econd = (BPM * steps_per_quarter) / 60
   samples_per_step = AUDIO_RATE_HZ / steps_per_second
*/
static void seq_update_timing(void)
{
   unsigned long denom;

   // protect against invalid runtime config
   if (s_steps_per_quarter == 0) s_steps_per_quarter = 1;

   // widen math to unsigned long before multiply
   denom = (unsigned long)s_bpm * (unsigned long)s_steps_per_quarter;
   if (denom == 0UL) denom = 1UL;

   s_samples_per_step = (AUDIO_RATE_HZ * 60UL) / denom;
}

// Init sequencer state and timing baseline
void seq_init(void)
{
   s_bpm = BPM_DEFAULT;
   s_step = 0;
   s_pattern_steps = PATTERN_STEPS_DEFAULT;
   s_steps_per_quarter = STEPS_PER_QUARTER_DEFAULT;
   s_step_accum = 0;
   s_step_advanced_flag = 0;
   seq_update_timing();
}

// Emit one-tick edge flag when sep boundary is crossed
void seq_tick(void)
{
   s_step_advanced_flag = 0;
   s_step_accum++;

   if (s_step_accum >= s_samples_per_step)
   {
      // keeps the beat from drifting over many steps
      s_step_accum -= s_samples_per_step;

      s_step++;
      if (s_step >= s_pattern_steps) s_step = 0;

      s_step_advanced_flag = 1;
   }
}

// Runtime tempo update
void seq_set_bpm(unsigned int bpm)
{
   s_bpm = bpm;
   seq_update_timing();
}

// Normalize pattern-length to constrained values
void seq_set_pattern_steps(unsigned char steps)
{
   if (steps < STEPS_MIN) steps = STEPS_MIN;
   if (steps > STEPS_MAX) steps = STEPS_MAX;

   s_pattern_steps = steps;

   // keep current step valid if pattern length is reduced
   if (s_step >= s_pattern_steps) s_step = 0;
}

// Runtime step-resolution update
void seq_set_steps_per_quarter(unsigned char spq)
{
   s_steps_per_quarter = spq;
   seq_update_timing();
}

// Getters
unsigned int seq_get_bpm(void) { return s_bpm; }
unsigned char seq_get_step(void) { return s_step; }
unsigned char seq_get_pattern_steps(void) { return s_pattern_steps; }
unsigned char seq_get_steps_per_quarter(void) { return s_steps_per_quarter; }

// Return 1 on ticks where step is advanced, else 0
unsigned char seq_step_advanced(void) { return s_step_advanced_flag; }