#include <pic18.h>
#include "lcd_portd.h"
#include "lcd_helpers.h"
#include "project_config.h"
#include "sequencer.h"
#include "synth.h"
#include "ui_state.h"

// UI row templates (16 chars each for full LCD row writes)
static const char UI_STATUS_TEMPLATE[17] = "BPM:    MODE:   ";

static const char UI_MODE_EDIT_TEXT[4] = "EDT";
static const char UI_MODE_PLAY_TEXT[4] = "PLY";

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

// BPM edit state (active only in EDIT screen)
volatile unsigned char g_bpm_edit_active = 0U;
volatile unsigned char g_bpm_blink_visible = 1U;
volatile unsigned int g_bpm_blink_counter = 0U;

// UI screen and edit cursor blink state
// Default to edit mode 
volatile unsigned char g_ui_screen_mode = UI_SCREEN_DEFAULT;
volatile unsigned char g_edit_cursor_visible = 1U;
volatile unsigned int g_edit_blink_counter = 0U;

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
   //g_kick_pattern[0] = 1;
   g_kick_pattern[4] = 1;
   //g_kick_pattern[8] = 1;
   g_kick_pattern[12] = 1;

   //g_snare_pattern[4] = 1;
   //g_snare_pattern[12] = 1;

   /*
   g_hihat_pattern[1] = 1;
   g_hihat_pattern[2] = 1;
   g_hihat_pattern[3] = 1;
   g_hihat_pattern[4] = 1;
   g_hihat_pattern[5] = 1;
   g_hihat_pattern[6] = 1;
   g_hihat_pattern[8] = 1;
   g_hihat_pattern[10] = 1;
   g_hihat_pattern[14] = 1;
   */
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

// Convert synth note index to display character
static unsigned char ui_note_index_to_letter(unsigned char note_index)
{
   if (note_index == 0U) return '_';
   return (unsigned char)('A' + ((note_index - 1U) % 7U));
}

// Forward declaration needed because this helper is used before its definition.
static unsigned char ui_step_active_for_page(
   unsigned char step_index,
   unsigned char track_page);

/*
   Draw one 8-slot page on LCD row 0

   Row-0 layout:
   [0]W [1]window# [2]: [3]space [4..11]8 step slots [12]L [13..14]pattern len

   Slot symbol priority (highest to lowest):
   1) '*' = playhead (runtime currently playing step)
   2) 'C' = edit cursor (user-selected step)
   3) '#' = step has at least one drum hit
   4) '_' = empty step
*/

static void ui_draw_step_window(void)
{
   unsigned char col;
   unsigned char step_index;
   unsigned char window_start;
   unsigned char play_step;
   unsigned char window_number;
   unsigned char slot_char;
   unsigned char cursor_step;
   unsigned char track_page;
   unsigned char synth_note_index;
   unsigned char pattern_len;
   unsigned char tens;
   unsigned char ones;

   // Left edge of current page
   window_start = ui_state_get_window_start();

   play_step = ui_state_get_play_step();
   cursor_step = ui_state_get_selected_step();
   track_page = ui_state_get_track_page();

   // Human-readable page number 1..4 for 32-step max
   window_number = (unsigned char)((window_start / UI_WINDOW_SIZE) + 1U);

   // Draw row header
   lcd_move(0, 0); 
   
   // Render logic for different instrument pages
   switch (track_page)
   {
      case TRACK_PAGE_KICK:   lcd_print("K"); break;
      case TRACK_PAGE_SNARE:  lcd_print("S"); break;
      case TRACK_PAGE_HIHAT:  lcd_print("H"); break;
      case TRACK_PAGE_SYNTH:  lcd_print("Y"); break;
      default:                lcd_print("?"); break;
   }

   lcd_move(0, 1); lcd_write((unsigned char)('0' + window_number));
   lcd_move(0, 2); lcd_write(':');
   lcd_move(0, 3); lcd_write(' ');

   // Draw 8 visual slots - each LCD slot maps to pattern step
   for (col = 0; col < UI_WINDOW_SIZE; col++)
   {
      step_index = (unsigned char)(window_start + col);

      // Base slot content by page type
      if (track_page == TRACK_PAGE_SYNTH)
      {
         // Synth page: show note letter or rest
         synth_note_index = synth_get_note_at_step(step_index);
         slot_char = ui_note_index_to_letter(synth_note_index);
      }

      else
      {
         // Drum page: show selected track only
         slot_char = ui_step_active_for_page(step_index, track_page) ? '#' : '_';
      }

      /*
            Edit Screen
            Blink selected-step cursor only
      */
      if (g_ui_screen_mode == UI_SCREEN_EDIT)
      {
         if ((step_index == cursor_step) && g_edit_cursor_visible)
         {
            slot_char = UI_CURSOR_BLOCK_CHAR;
         }
      }
      
      // Play screen
      else
      {
         if (step_index == play_step) slot_char = '*';
      }

      lcd_move(0, (unsigned char)(UI_SLOT_FIRST_COL + col));
      lcd_write(slot_char);
   }

   // Show pattern length at right edge
   pattern_len = ui_state_get_pattern_steps();
   tens = (unsigned char)(pattern_len / 10U);
   ones = (unsigned char)(pattern_len % 10U);

   lcd_move(0, 12); lcd_write(' '); // Clear space before pattern display
   lcd_move(0, 13); lcd_write((unsigned char)('0' + tens));
   lcd_move(0, 14); lcd_write((unsigned char)('0' + ones));
}

/*
   Return 1 if selected track page has active step at step_index
   For real time render of selected track
*/
static unsigned char ui_step_active_for_page(
   unsigned char step_index, 
   unsigned char track_page)
{
   switch (track_page)
   {
      case TRACK_PAGE_KICK:  return g_kick_pattern[step_index] ? 1U : 0U;
      case TRACK_PAGE_SNARE: return g_snare_pattern[step_index] ? 1U : 0U;
      case TRACK_PAGE_HIHAT: return g_hihat_pattern[step_index] ? 1U : 0U;
      default:               return 0U;
   }
}

// Draw bottom status row: BPM value 
static void ui_draw_status_row(void)
{
   unsigned int bpm;

   // BPM decimal digits: hundreds, tens, ones
   unsigned char bpm_hundreds;
   unsigned char bpm_tens;
   unsigned char bpm_ones;

   // Full row template
   lcd_move(1, 0); lcd_print(UI_STATUS_TEMPLATE);

   bpm = seq_get_bpm();
   if (bpm > 999U) bpm = 999U;

   bpm_hundreds = (unsigned char)(bpm / 100U);
   bpm_tens = (unsigned char)((bpm / 10U) % 10U);
   bpm_ones = (unsigned char)(bpm % 10U);

   // If BPM edit is active and blink phase is "off" hide BPM
   if (g_bpm_edit_active && !g_bpm_blink_visible)
   {
      lcd_move(1, 4);
      lcd_write(' ');
      lcd_write(' ');
      lcd_write(' ');
   }

   else 
   {
      // Write BPM at cols 4..6
      lcd_move(1, 4);
      lcd_write(bpm_hundreds ? (unsigned char)('0' + bpm_hundreds) : ' ');
      lcd_write((bpm_hundreds || bpm_tens) ? (unsigned char)('0' + bpm_tens) : ' ');
      lcd_write((unsigned char)('0' + bpm_ones));
   }
   
   // Show current screen mode at cols 13..15
   lcd_move(1, 13);
   if (g_ui_screen_mode == UI_SCREEN_EDIT) lcd_print(UI_MODE_EDIT_TEXT);
   else lcd_print(UI_MODE_PLAY_TEXT);
}

static void ui_service_status(void)
{
   static unsigned char last_displayed_step = LCD_STEP_INVALID;
   static unsigned char last_window_start = LCD_STEP_INVALID;
   unsigned char current_step;
   unsigned char current_window_start;

   if (!g_ui_update_flag) return;

   g_ui_update_flag = 0;
   current_step = ui_state_get_play_step();
   current_window_start = ui_state_get_window_start();

   if (current_step != last_displayed_step || 
      (current_window_start != last_window_start)) 
   {
      last_displayed_step = current_step;
      last_window_start = current_window_start;

      ui_draw_step_window();
      ui_draw_status_row();
   }
}

// Advance page: KICK -> SNARE -> HIHAT -> SYNTH -> KICK
static void ui_next_track_page(void)
{
   unsigned char page;

   page = ui_state_get_track_page();
   page++; 

   if (page > TRACK_PAGE_SYNTH) page = TRACK_PAGE_KICK;

   ui_state_set_track_page(page);
   g_ui_update_flag = 1; // redraw immediately
}

// Toggle BPM edit mode (EDIT screen only)
static void ui_toggle_bpm_edit_mode(void)
{
   // Safety gate: BPM edit is only valid on EDIT screen
   if (g_ui_screen_mode != UI_SCREEN_EDIT) return;

   g_bpm_edit_active ^= 1U;

   // Reset blink so BPM is immediately visible when toggled
   g_bpm_blink_counter = 0U;
   g_bpm_blink_visible = 1U;

   // Request redraw so user sees mode change immediately
   g_ui_update_flag = 1U;
}

// Apply signed BPM delta with min/max clamp and request redraw
static void ui_adjust_bpm(signed char delta)
{
   unsigned int bpm_now;

   // Safety gate: only allows adjust while BPM edit mode is active
   if (!g_bpm_edit_active) return;

   bpm_now = seq_get_bpm();

   if (delta > 0)
   {
      if (bpm_now < BPM_MAX)
      {
         bpm_now = (unsigned int)(bpm_now + BPM_STEP);
         
         // Top level clamp
         if (bpm_now > BPM_MAX) bpm_now = BPM_MAX;
      }
   }

   else if (delta < 0)
   {
      if (bpm_now > BPM_MIN)
      {
         if (bpm_now > BPM_STEP) bpm_now = (unsigned int)(bpm_now - BPM_STEP);
         else bpm_now = BPM_MIN;
         
         // Bottom level clamp
         if (bpm_now < BPM_MIN) bpm_now = BPM_MIN;
      }
   }

   seq_set_bpm(bpm_now);
   g_ui_update_flag = 1U;
}

static void ui_toggle_screen_mode(void)
{
   if (g_ui_screen_mode == UI_SCREEN_EDIT) g_ui_screen_mode = UI_SCREEN_PLAY;
   else g_ui_screen_mode = UI_SCREEN_EDIT;

   // Reset blink state so edit cursor returns immediately
   g_edit_blink_counter = 0U;
   g_edit_cursor_visible = 1U;

   // Request redraw so mode switch appears instantly
   g_ui_update_flag = 1U;
}

// Poll RB0..RB7 on rising edges - edit controls only active in edit mode
static void ui_poll_page_button(void)
{
   // Previous sampled states - static keeps value across iterations
   static unsigned char rb0_prev = 0U;
   static unsigned char rb1_prev = 0U;
   static unsigned char rb2_prev = 0U;
   static unsigned char rb3_prev = 0U;
   static unsigned char rb4_prev = 0U;
   static unsigned char rb6_prev = 0U;
   static unsigned char rb7_prev = 0U;
   
   // EDIT mode: allow page and cursor navigation
   if (g_ui_screen_mode == UI_SCREEN_EDIT)
   {
      // RB0: cycle instrument page (K -> S -> H -> Y -> K)
      if (RB0 && !rb0_prev)
      {
         wait_ms(20);
         if (RB0) ui_next_track_page();
      }

      // RB1: move edit cursor left
      if (RB1 && !rb1_prev)
      {
         wait_ms(20);
         if (RB1)
         {
            ui_state_cursor_left();
            g_ui_update_flag = 1U; // request LCD redraw
         }
      }

      // RB2: move edit cursor right
      if (RB2 && !rb2_prev)
      {
         wait_ms(20);
         if (RB2)
         {
            ui_state_cursor_right();
            g_ui_update_flag = 1U;
         }
      }

      // RB4: toggle BPM edit mode
      if (RB4 && !rb4_prev)
      {
         wait_ms(20);
         if (RB4) ui_toggle_bpm_edit_mode();
      }

      // RB6: BPM down 
      if (RB6 && !rb6_prev)
      {
         wait_ms(8); // allows for quick button presses
         if (RB6) ui_adjust_bpm((signed char)(-((signed int)BPM_STEP)));
      }

      // RB7: BPM up
      if (RB7 && !rb7_prev)
      {
         wait_ms(8);
         if (RB7) ui_adjust_bpm(BPM_STEP);
      }
   }
   
   // RB3: toggle screen mode (EDIT <--> PLAY)
   if (RB3 && !rb3_prev)
   {
      wait_ms(20);
      if (RB3) ui_toggle_screen_mode();
   }

   // Always update previous-state latches
   rb0_prev = RB0 ? 1U : 0U;
   rb1_prev = RB1 ? 1U : 0U;
   rb2_prev = RB2 ? 1U : 0U;
   rb3_prev = RB3 ? 1U : 0U;
   rb4_prev = RB4 ? 1U : 0U;
   rb6_prev = RB6 ? 1U : 0U;
   rb7_prev = RB7 ? 1U : 0U;
}

// Interrupt handler at AUDIO_RATE_HZ
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

         // Keep UI playhead in sync w sequencer runtime step
         ui_state_set_play_step(step_now);
         g_ui_update_flag = 1; // request LCD refresh on step edge

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

      // Render one audio sample every ISR tick
      update_audio_output();

      // Blink BPM field only when BPM edit mode is active
      if(g_bpm_edit_active)
      {
         g_bpm_blink_counter++;

         // Toggle BPM visibility at configured blink period
         if (g_bpm_blink_counter >= BPM_BLINK_TICKS)
         {
            g_bpm_blink_counter = 0U;
            g_bpm_blink_visible ^= 1U; // toggle
            g_ui_update_flag = 1U; // force LCD refresh for blink
         }
      }

      else
      {
         // Keep BPM field steadily visible when not editing
         g_bpm_blink_counter = 0U;
         g_bpm_blink_visible = 1U;
      }

      // Blink edit cursor only while on edit screen
      if (g_ui_screen_mode == UI_SCREEN_EDIT)
      {
         g_edit_blink_counter++;

         // Toggle cursor visibility at configured blink period
         if(g_edit_blink_counter >= EDIT_CURSOR_BLINK_TICKS)
         {
            g_edit_blink_counter = 0U;
            g_edit_cursor_visible ^= 1U;

            // Force UI refresh even if sequencer step did not change
            g_ui_update_flag = 1U;
         }
      }

      // Update UI at a slower rate than audio processing
      // g_ui_divider++;

      // When enough ticks pass, main loop refresh LCD
      // if(g_ui_divider >= UI_UPDATE_DIVIDER_TICKS) // ~10 ms
      // {
         // g_ui_divider = 0;
         // g_ui_update_flag = 1;
      // }
   }
}

void main(void)
{
   ADCON1 = 0x0F;
   TRISA = 0x00;
   TRISB = 0x0F; // RB0..RB3 are inputs
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

   // Init UI state from current sequencer config
   ui_state_init();
   ui_state_set_pattern_steps(seq_get_pattern_steps());
   ui_state_set_play_step(seq_get_step());

   // Init demo drum and synth patterns
   init_demo_pattern();
   synth_init();
   //synth_load_demo_pattern();

   // Draw initial UI frame
   lcd_move(0, 0); lcd_print("                ");
   lcd_move(1, 0); lcd_print("                ");
   ui_draw_step_window();
   ui_draw_status_row();

   g_ui_update_flag = 1;

   // Start interrupts only after UI baseline is visible.
   PEIE = 1;
   GIE  = 1;

   // Only update LCD when ISR requests
   while (1) 
   {
      ui_poll_page_button();
      ui_service_status();
   }
}
