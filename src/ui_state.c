#include <pic18.h>
#include "project_config.h"
#include "ui_state.h"

// Internal UI state
static unsigned char s_mode = UI_MODE_DRUM;
static unsigned char s_pattern_steps = PATTERN_STEPS_DEFAULT;
static unsigned char s_play_step = 0; // Current sequencer step
static unsigned char s_window_start = 0;
static unsigned char s_cursor_col = 0; // Current cursor location
static unsigned char s_octave = OCTAVE_DEFAULT;
static unsigned char s_track_page = TRACK_PAGE_DEFAULT;

// Accept exact values only - else default value
static unsigned char ui_normalize_pattern_steps(unsigned char steps)
{
   switch(steps)
   {
      case PATTERN_STEPS_8:  return PATTERN_STEPS_8;
      case PATTERN_STEPS_16: return PATTERN_STEPS_16;
      case PATTERN_STEPS_24: return PATTERN_STEPS_24;
      case PATTERN_STEPS_32: return PATTERN_STEPS_32;
      default:               return PATTERN_STEPS_DEFAULT;
   }
}

// Keep window and cursor valid after pattern-length changes
static void ui_fix_selection_bounds(void)
{
   unsigned char max_window_start;
   unsigned char selected_step;

   // Pattern can't be smaller than one visible page
   if (s_pattern_steps < UI_WINDOW_SIZE) s_pattern_steps = UI_WINDOW_SIZE;

   // Last legal page start for pattern length
   max_window_start = (unsigned char)(s_pattern_steps - UI_WINDOW_SIZE);
   if (s_window_start > max_window_start) s_window_start = max_window_start;

   // Cursor must stay inside 0..7
   if (s_cursor_col >= UI_WINDOW_SIZE) s_cursor_col = (unsigned char)(UI_WINDOW_SIZE - 1U);

   // Absolute selected step = page base + cursor column
   selected_step = (unsigned char)(s_window_start + s_cursor_col);

   // If selected step is outside pattern, move cursor left
   if (selected_step >= s_pattern_steps)
   {
      s_cursor_col = (unsigned char)(s_pattern_steps - s_window_start - 1U);
   }
}

void ui_state_init(void)
{
   s_mode = UI_MODE_DRUM;
   s_pattern_steps = ui_normalize_pattern_steps(PATTERN_STEPS_DEFAULT);
   s_play_step = 0;
   s_window_start = 0;
   s_cursor_col = 0;
   s_octave = OCTAVE_DEFAULT;
   s_track_page = TRACK_PAGE_DEFAULT; 

   ui_fix_selection_bounds();
}

// Set current UI track page
void ui_state_set_track_page(unsigned char page)
{
   // Clamp invalid values to known-safe default page
   if (page > TRACK_PAGE_SYNTH) page = TRACK_PAGE_DEFAULT;
   s_track_page = page;
}

// Read track page value
unsigned char ui_state_get_track_page(void) { return s_track_page; }

void ui_state_set_mode(unsigned char mode)
{
   // If mode entered is other than synth or drum default to drum mode
   if (mode > UI_MODE_SYNTH) mode = UI_MODE_DRUM;
   s_mode = mode;
}

unsigned char ui_state_get_mode(void) { return s_mode; }

void ui_state_set_pattern_steps(unsigned char steps)
{
   // Snap to accepted pattern length values
   s_pattern_steps = ui_normalize_pattern_steps(steps);
   ui_fix_selection_bounds();
}

unsigned char ui_state_get_pattern_steps(void) { return s_pattern_steps; }

void ui_state_set_play_step(unsigned char step)
{
   if (s_pattern_steps == 0U) s_pattern_steps = UI_WINDOW_SIZE;
   s_play_step = (unsigned char)(step % s_pattern_steps);

   // Auto-follow playback page:
   // steps 0..7 -> window 0, 8..15 -> window 8, etc.
   s_window_start = (unsigned char)((s_play_step / UI_WINDOW_SIZE) * UI_WINDOW_SIZE);
}

unsigned char ui_state_get_play_step(void) { return s_play_step; }

void ui_state_cursor_left(void)
{
   // Move cursor left if in valid step above 0
   if (s_cursor_col > 0U) s_cursor_col--;

   else if (s_window_start >= UI_WINDOW_SIZE)
   {
      s_window_start = (unsigned char)(s_window_start - UI_WINDOW_SIZE);
      s_cursor_col = (unsigned char)(UI_WINDOW_SIZE - 1U);
   }
}

void ui_state_cursor_right(void)
{
   unsigned char selected_step;
   selected_step = (unsigned char)(s_window_start + s_cursor_col);

   if ((s_cursor_col < (unsigned char)(UI_WINDOW_SIZE - 1U)) &&
      ((unsigned char)(selected_step + 1U) < s_pattern_steps)) s_cursor_col++;
   
   else if ((unsigned char)(s_window_start + UI_WINDOW_SIZE) < s_pattern_steps)
   {
      s_window_start = (unsigned char)(s_window_start + UI_WINDOW_SIZE);
      s_cursor_col = 0;
   }
}

void ui_state_page_prev(void)
{
   if (s_window_start >= UI_WINDOW_SIZE)
   {
      s_window_start = (unsigned char)(s_window_start - UI_WINDOW_SIZE);
   }

   ui_fix_selection_bounds();
}

void ui_state_page_next(void)
{
   if ((unsigned char)(s_window_start + UI_WINDOW_SIZE) < s_pattern_steps)
   {
      s_window_start = (unsigned char)(s_window_start + UI_WINDOW_SIZE);
   }

   ui_fix_selection_bounds();
}

unsigned char ui_state_get_window_start(void) { return s_window_start; }
unsigned char ui_state_get_cursor_col(void) { return s_cursor_col; }
unsigned char ui_state_get_selected_step(void) { return (unsigned char)(s_window_start + s_cursor_col); }
unsigned char ui_state_get_octave(void) { return s_octave; }

// Set octave while enforcing top/bottom params
void ui_state_set_octave(unsigned char octave)
{
   if (octave < OCTAVE_MIN) octave = OCTAVE_MIN;
   if (octave > OCTAVE_MAX) octave = OCTAVE_MAX;
   s_octave = octave;
}
