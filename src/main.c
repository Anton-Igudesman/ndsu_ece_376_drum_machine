#include <pic18.h>
#include "lcd_portd.h"
#include "lcd_helpers.h"
#include "project_config.h"
#include "sequencer.h"
#include "synth.h"

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
volatile unsigned char g_ui_update_flag = 0;
volatile unsigned char g_ui_divider = 0;

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
   signed int synth_sample;

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

   // Synth channel output (RC1/CCP2)
   synth_sample = synth_tick_sample();
   pwm_set_duty_ccp2(to_pwm_duty(synth_sample));
}

static void ui_draw_status_labels(void)
{
   lcd_move(0, 0); lcd_print("STEP: ");
   lcd_move(0, 9); lcd_print("LEN: ");
   lcd_move(1, 0); lcd_print("BPM: ");
   lcd_move(1, 9); lcd_print("SPQ:");
}

static void ui_service_status(void)
{
   static unsigned char last_displayed_step = LCD_STEP_INVALID;
   unsigned char current_step;

   if (!g_ui_update_flag) return;

   g_ui_update_flag = 0;
   current_step = seq_get_step();

   if (current_step != last_displayed_step)
   {
      last_displayed_step = current_step;

      lcd_move(0, 5);  lcd_out(current_step, 2, 0);
      lcd_move(0, 13); lcd_out(seq_get_pattern_steps(), 2, 0);
      lcd_move(1, 4);  lcd_out(seq_get_bpm(), 3, 0);
      lcd_move(1, 13); lcd_out(seq_get_steps_per_quarter(), 1, 0);
   }
}

// Interrupt handler at 25 KHz
void interrupt IntServe(void)
{
   if (TMR0IF)
   {
      timer0_load();
      TMR0IF = 0;

      //  Sequencer timing by one audio tick
      seq_tick();

      // Only run trigger logic on true step boundaries
      if (seq_step_advanced())
      {
         unsigned char step_now = seq_get_step();

         // Pattern lookups for step
         if (g_kick_pattern[step_now]) g_kick_trigger = 1;
         if (g_snare_pattern[step_now]) g_snare_trigger = 1;
         if (g_hihat_pattern[step_now]) g_hihat_trigger = 1;

         // Load synth note for the new step
         synth_on_step(step_now);

         // Convert triggers into envelope starts
         if (g_kick_trigger)
         {
            g_kick_env = KICK_ENV_DEFAULT;
            g_kick_half_period = KICK_HALF_PERIOD_START;
            g_kick_counter = g_kick_half_period;
            g_kick_polarity = 1;
            g_kick_trigger = 0;
         }

         if (g_snare_trigger)
         {
            g_snare_env = SNARE_ENV_DEFAULT;
            g_snare_trigger = 0;
         }

         if (g_hihat_trigger)
         {
            g_hihat_env = HIHAT_ENV_DEFAULT;
            g_hihat_trigger = 0;
         }
      }

      // Render one audio sameple every ISR tick
      update_audio_output();

      // Update UI at a slower rate than audio processing
      g_ui_divider++;

      // When enough ticks pass, main loop refresh LCD
      if(g_ui_divider >= UI_UPDATE_DIVIDER_TICKS) // ~10 ms
      {
         g_ui_divider = 0;
         g_ui_update_flag = 1;
      }
   }
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

   // Init sequencer module with project defaults
   seq_init();
   seq_set_pattern_steps(PATTERN_STEPS_DEFAULT);
   seq_set_steps_per_quarter(STEPS_PER_QUARTER_DEFAULT);
   seq_set_bpm(BPM_DEFAULT);

   // Init demo drum and synth patterns
   init_demo_pattern();
   synth_init();
   synth_load_demo_pattern();

   PEIE = 1;
   GIE  = 1;

   // Draw UI
   lcd_move(1, 0); lcd_print("                ");
   ui_draw_status_labels();
   g_ui_update_flag = 1;

   // Only update LCD when ISR requests
   while (1) 
   {
     ui_service_status();
   }
}
