#include <pic18.h>
#include "synth.h"
#include "project_config.h"

/*
   Note index mapping:
   0 = rest
   1..7     = A3..G3
   8..14    = A4..G4
   15..21   = A5..G5 
*/


static unsigned char s_synth_notes[32] = {0};

/*
   These numbers control synth pitch

   The synth flips between + and - every "half-period" ticks
   Larger number = slower flips = lower note
   Smaller number = faster flips = higher note

   Quick formula used to pick each number:
   half_period_ticks ~= AUDIO_RATE_HZ / (2 * note_frequency)
*/

static const unsigned int s_note_half_period[22] = 
{
   0, 

   // A3, B3, C3, D3, E3, F3, G3
   57, 51, 48, 43, 38, 36, 32,

   // A4, B4, C4, D4, E4, F4, G4
   28, 25, 24, 21, 19, 18, 16,

   // A5, B5, C5, D5, E5, F5, G5
   14, 13, 12, 11, 10, 9, 8
};

static unsigned int synth_note_half_period(unsigned char note_index)
{
   if (note_index > 21) return 0; // invalid index returns silence
   return s_note_half_period[note_index];
}

static unsigned char s_current_note = 0;
static unsigned int s_half_period = 0;
static unsigned int s_counter = 0;
static signed char s_polarity = 1;
static unsigned int s_env = 0;

void synth_init(void)
{
   unsigned char i;

   // Set all notes to 0
   for (i = 0; i < 32; i++) s_synth_notes[i] = 0;

   s_current_note = 0;
   s_half_period = 0;
   s_counter = 0;
   s_polarity = 1;
   s_env = 0;
}

void synth_load_demo_pattern(void)
{
   unsigned char i;
   for (i = 0; i < 32; i++) s_synth_notes[i] = 0;
   
   s_synth_notes[0] = 8;     // A4
   s_synth_notes[2] = 10;    // C4
   s_synth_notes[4] = 12;    // E4
   s_synth_notes[6] = 14;    // G4
   s_synth_notes[8] = 15;    // A5
   s_synth_notes[10] = 17;   // C5
   s_synth_notes[12] = 19;   // E5
   s_synth_notes[14] = 21;   // G5
}

void synth_on_step(unsigned char step_index)
{
   s_current_note = s_synth_notes[step_index];

   if (s_current_note == 0)
   {
      // No sound state
      s_env = 0;
      s_half_period = 0;
      s_counter = 0;
      s_polarity = 1;
   }
   else
   {
      // Load note timing and start envelope
      s_half_period = synth_note_half_period(s_current_note);
      s_counter = s_half_period;
      s_polarity = 1;
      s_env = SYNTH_ENV_DEFAULT;
   }
}

signed int synth_tick_sample(void)
{
   signed int sample = 0;

   if((s_env > 0) && (s_half_period > 0))
   {
      if (s_counter == 0)
      {
         s_counter = s_half_period;
         s_polarity = -s_polarity; // oscillating over half period
      }
      
      else s_counter--;

      // Square-wave sample scaled by envelope
      sample = (signed int)s_polarity * (signed int)(s_env >> 2);

      // Linear decay toward silence
      if (s_env > SYNTH_ENV_DECAY_PER_TICK) s_env -= SYNTH_ENV_DECAY_PER_TICK;
      else s_env = 0;
   }

   // Clamp values to defined parameters
   if (sample > AUDIO_SAMPLE_MAX) sample = AUDIO_SAMPLE_MAX;
   if (sample < AUDIO_SAMPLE_MIN) sample = AUDIO_SAMPLE_MIN;

   return sample;
}
