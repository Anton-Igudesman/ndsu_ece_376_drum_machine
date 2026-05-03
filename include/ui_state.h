#ifndef UI_STATE_H
#define UI_STATE_H

void ui_state_init(void);

// UI mode: drum edit vs synth edit
void ui_state_set_mode(unsigned char mode);
unsigned char ui_state_get_mode(void);

// Pattern length currently active (8/16/24/32)
void ui_state_set_pattern_steps(unsigned char steps);
unsigned char ui_state_get_pattern_steps(void);

// Current sequencer play step - for playhead logic
void ui_state_set_play_step(unsigned char step);
unsigned char ui_state_get_play_step(void);

// Edit cursor movement inside current 8-step window
void ui_state_cursor_left(void);
void ui_state_cursor_right(void);

// Move to previous/next 8-step window
void ui_state_page_prev(void);
void ui_state_page_next(void);

// Read current edit viewport
unsigned char ui_state_get_window_start(void);
unsigned char ui_state_get_cursor_col(void);
unsigned char ui_state_get_selected_step(void);

// Synth edit context (A-G with current octave)
void ui_state_set_octave(unsigned char octave);
unsigned char ui_state_get_octave(void);

void ui_state_set_track_page(unsigned char page);
unsigned char ui_state_get_track_page(void);

#endif