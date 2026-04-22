#include <pic18.h>
#include "lcd_portd.h"
#include "lcd_helpers.h"

#define FOSC_HZ        40000000UL
#define FCY_HZ         (FOSC_HZ / 4UL)
#define AUDIO_RATE_HZ  25000UL
#define TMR0_RELOAD    (65536UL - (FCY_HZ / AUDIO_RATE_HZ))

volatile unsigned long g_audio_ticks = 0;
volatile unsigned int g_bpm = 120; // configurable BPM steps
volatile unsigned char g_step = 0;
volatile unsigned long g_samples_per_step = 0;
volatile unsigned long g_step_accum = 0;
volatile unsigned char g_pattern_steps = 16; // 8/16/24/32
volatile unsigned char g_steps_per_quarter = 2; // timing resolution

// Pattern maps for instruments
volatile unsigned char g_kick_pattern[32] = {0};
volatile unsigned char g_snare_pattern[32] = {0};
volatile unsigned char g_hihat_pattern[32] = {0};

// Triggers for output
volatile unsigned char g_kick_trigger = 0;
volatile unsigned char g_snare_trigger = 0;
volatile unsigned char g_hihat_trigger = 0;

// Drum state globals
volatile unsigned int g_kick_env = 0;
volatile unsigned int g_snare_env = 0;
volatile unsigned int g_hihat_env = 0;
volatile unsigned int g_noise_lfsr = 0xACE1;

// Kick attributes
volatile unsigned int g_kick_half_period = 120;
volatile unsigned int g_kick_counter = 0;
volatile signed char g_kick_polarity = 1;

// Demo pattern
static void init_demo_pattern(void)
{
   unsigned char i;

   for (i = 0; i < 32; i++)
   {
      // Reset all steps
      g_kick_pattern[i] = 0;
      g_snare_pattern[i] = 0;
      g_hihat_pattern[i] = 0;
   }

   // 16-step demo groove
   g_kick_pattern[0] = 1;
   g_kick_pattern[2] = 1;
   g_kick_pattern[3] = 1;
   g_kick_pattern[8] = 1;

   //g_snare_pattern[4] = 1;
   //g_snare_pattern[12] = 1;

   g_hihat_pattern[1] = 1;
   g_hihat_pattern[2] = 1;
   g_hihat_pattern[3] = 1;
   g_hihat_pattern[4] = 1;
   g_hihat_pattern[5] = 1;
   g_hihat_pattern[6] = 1;
   g_hihat_pattern[8] = 1;
   g_hihat_pattern[10] = 1;
   g_hihat_pattern[14] = 1;
}

/*
   Set CCP1 PWM level
   duty10 is a 10-bit value (0..511 when PR2 = 0x7F)
      - PR2 sets PWM period length

   Higher duty -> higher avg output voltage
*/
static void pwm_set_duty_ccp1(unsigned int duty10)
{
   if (duty10 > 511U) duty10 = 511U;

   // Upper 8 bits of duty go to CCPR1L
   CCPR1L = (unsigned char)(duty10 >> 2);

   // Lower 2 bits of duty go to DC1B1:DC1B0
   DC1B1  = (unsigned char)((duty10 >> 1) & 0x01);
   DC1B0  = (unsigned char)(duty10 & 0x01);
}

// Same as above, for CCP2
static void pwm_set_duty_ccp2(unsigned int duty10)
{
   if (duty10 > 511U) duty10 = 511U;
   CCPR2L = (unsigned char)(duty10 >> 2);
   DC2B1  = (unsigned char)((duty10 >> 1) & 0x01);
   DC2B0  = (unsigned char)(duty10 & 0x01);
}

// Set both PWM channels to neutral output (50%)
static void pwm_set_midpoint(void)
{
   pwm_set_duty_ccp1(256U);
   pwm_set_duty_ccp2(256U);
}

/*
   Configure PWM hardware:
   - CCP1 on RC2 (drum channel)
   - CCP2 on RC1 (synth channel)
*/
static void audio_pwm_init(void)
{
   PR2     = 0x7F;
   T2CON   = 0x04; // Timer2 on, prescale = 1
   CCP1CON = 0x0C; // CCP1 in PWM mode
   CCP2CON = 0x0C; // CCP2 in PWM mode
   pwm_set_midpoint();
}

// Load Timer0 so overflow when at target interrupt rate
static void timer0_load(void)
{
   TMR0H = (unsigned char)(TMR0_RELOAD >> 8);
   TMR0L = (unsigned char)(TMR0_RELOAD & 0xFF);
}

// Configure Timer0 as the periodic audio tick source
static void audio_tick_init(void)
{

   /*
      T0CON = 0x88:
      - Timer0 on
      - 16-bit mode
      - internal clock source (Fcy)
      - no prescaler
   */
   T0CON = 0x88;
   timer0_load();
   TMR0IF = 0; // Clear pending flag
   TMR0IE = 1; // enable Timer0 interrupt
}

static signed int clamp_audio(signed int x)
{
   if (x > 255) return 255;
   if (x < -256) return -256;
   return x;
}

static unsigned int to_pwm_duty(signed int sample)
{
   // map -256..255 to 0..511
   return (unsigned int)(sample + 256);
}

static void update_audio_output(void)
{
   signed int drum_sample = 0;
   signed int noise;

   // Linear Feedback Shift Register - noise machine
   g_noise_lfsr = (g_noise_lfsr >> 1) ^ (-(signed int)(g_noise_lfsr & 1U) & 0xB400U);

   // Convert LFSR LSB to near-zero mean - polar
   noise = (g_noise_lfsr & 1U) ? 1 : -1;

   // Kick sound
   if (g_kick_env > 0)
   {
      // Square-wave kick oscillator at audio tick rate:
      // freq ~= AUDIO_RATE_HZ / (2 * g_kick_half_period)
      if (g_kick_counter == 0)
      {
         g_kick_counter = g_kick_half_period;
         g_kick_polarity = -g_kick_polarity;

         // Pitch sweep down: increase half-period over time
         if (g_kick_half_period < 180) g_kick_half_period++;
      }

      else g_kick_counter--;

      drum_sample += (signed int)g_kick_polarity * (signed int) (g_kick_env >> 2); // Small scale
      if (g_kick_env > 4) g_kick_env -= 4; else g_kick_env = 0; // Decay per tick
   }

   //Snare sound
   if (g_snare_env > 0)
   {
      drum_sample += noise * (signed int)(g_snare_env >> 2); // Noise amplitude
      if (g_snare_env > 8) g_snare_env -= 8; else g_snare_env = 0; // Decay per tick
   }

   // Hi-hat
   if (g_hihat_env > 0)
   {
      drum_sample += noise * (signed int)(g_hihat_env >> 2); 
      if (g_hihat_env > 20) g_hihat_env -= 20; else g_hihat_env = 0; // fastest decay
   }

   // Keep mixed sample within supported sign range
   drum_sample = clamp_audio(drum_sample);

   // Drum channel output (RC2/CCP1): signed sample -> 10-bit PWM duty
   pwm_set_duty_ccp1(to_pwm_duty(drum_sample));

   // Idle synth currently
   pwm_set_duty_ccp2(256U);
}

// Interrupt handler at 25 KHz
void interrupt IntServe(void)
{
   if (TMR0IF)
   {
      timer0_load();
      TMR0IF = 0;

      g_audio_ticks++;
      g_step_accum++;

      if (g_step_accum >= g_samples_per_step)
      {
         // Preserve phase
         g_step_accum -= g_samples_per_step;
         g_step++;
         
         // wrap around selected pattern length
         if (g_step >= g_pattern_steps) g_step = 0;

         // Trigger checks
         if (g_kick_pattern[g_step]) g_kick_trigger = 1;
         if (g_snare_pattern[g_step]) g_snare_trigger = 1;
         if (g_hihat_pattern[g_step]) g_hihat_trigger = 1;

         if (g_kick_trigger) 
         {
            g_kick_env = 1100;
            g_kick_half_period = 45;
            g_kick_counter = g_kick_half_period;
            g_kick_polarity = 1; 
            g_kick_trigger = 0;
         }
         if (g_snare_trigger) {g_snare_env = 1000; g_snare_trigger = 0;}
         if (g_hihat_trigger) {g_hihat_env = 500; g_hihat_trigger = 0;}
      }
      update_audio_output();
   }
}

// Convert BPM into ISR ticks per sequencer step
static void update_step_timing(void)
{
   /*
      samples per step = (AUDIO_RATE x 60) / (BPM x steps per quarter)
   */

   // Safety clamp
   if (g_steps_per_quarter == 0) g_steps_per_quarter = 1;

   unsigned long denom = (unsigned long)g_bpm * (unsigned long) g_steps_per_quarter;
   if (denom == 0UL) denom = 1UL;
   g_samples_per_step = (AUDIO_RATE_HZ * 60UL) / denom;
}

void main(void)
{
   ADCON1 = 0x0F;
   TRISA = 0x00;
   TRISB = 0x00;
   TRISC = 0x00;
   TRISD = 0x00;
   TRISE = 0x00;

   lcd_init(LCD_DEFAULT);
   lcd_move(0, 0); lcd_print("READY");
   lcd_move(1, 0); lcd_print("PRE-ISR OK");
   wait_ms(500);

   audio_pwm_init();
   audio_tick_init();

   // Safety clamp
   if (g_pattern_steps < 8) g_pattern_steps = 8;
   if (g_pattern_steps > 32) g_pattern_steps = 32;

   update_step_timing();
   init_demo_pattern();

   PEIE = 1;
   GIE  = 1;

   // clear bottom row
   lcd_move(1, 0); lcd_print("                ");
   lcd_move(0, 0); lcd_print("STEP: ");
   lcd_move(0, 9); lcd_print("LEN: ");
   lcd_move(1, 0); lcd_print("BPM: ");
   lcd_move(1, 9); lcd_print("SPQ:");

   while (1) 
   {
      static unsigned char last_displayed_step = 255;
      unsigned char current_step = g_step;

      if (current_step != last_displayed_step)
      {
         last_displayed_step = current_step;

         lcd_move(0, 5); lcd_out(current_step, 2, 0);
         lcd_move(0, 13); lcd_out(g_pattern_steps, 2, 0);
         lcd_move(1, 4); lcd_out(g_bpm, 3, 0);
         lcd_move(1, 13); lcd_out(g_steps_per_quarter, 1, 0);
      }
   }
}
