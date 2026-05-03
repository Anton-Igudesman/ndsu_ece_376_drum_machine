#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#define TRACK_PAGE_KICK             0U
#define TRACK_PAGE_SNARE            1U
#define TRACK_PAGE_HIHAT            2U
#define TRACK_PAGE_SYNTH            3U
#define TRACK_PAGE_DEFAULT          TRACK_PAGE_KICK

#define FOSC_HZ                     40000000UL
#define FCY_HZ                      (FOSC_HZ / 4UL)
#define AUDIO_RATE_HZ               10000UL
#define TMR0_RELOAD                 (65536UL - (FCY_HZ / AUDIO_RATE_HZ))
#define STEPS_MIN                   8U
#define STEPS_MAX                   32U
#define AUDIO_SAMPLE_MAX            255
#define AUDIO_SAMPLE_MIN            (-256)
#define UI_UPDATE_DIVIDER_TICKS     100U

#define BPM_DEFAULT                 120U
#define PATTERN_STEPS_DEFAULT       16U
#define STEPS_PER_QUARTER_DEFAULT   2U
#define LCD_STEP_INVALID            32U
#define SYNTH_ENV_DECAY_PER_TICK    2U

#define KICK_ENV_DEFAULT            1100U
#define SNARE_ENV_DEFAULT           1000U
#define HIHAT_ENV_DEFAULT           500U
#define SYNTH_ENV_DEFAULT           700U
#define KICK_HALF_PERIOD_START      45U

// UI definitions
#define UI_MODE_DRUM                0U
#define UI_MODE_SYNTH               1U
#define UI_WINDOW_SIZE              8U
#define OCTAVE_MIN                  3U
#define OCTAVE_MAX                  5U
#define OCTAVE_DEFAULT              4U
#define UI_SLOT_FIRST_COL           4U
#define UI_SCREEN_EDIT              0U
#define UI_SCREEN_PLAY              1U
#define UI_SCREEN_DEFAULT           UI_SCREEN_EDIT
#define UI_CURSOR_BLOCK_CHAR        ((unsigned char)0xFF)

#define EDIT_CURSOR_BLINK_TICKS     1250U

// BPM definitions
#define BPM_MIN                     60U
#define BPM_MAX                     200U
#define BPM_STEP                    1U
#define BPM_BLINK_TICKS             1250U

#define PATTERN_STEPS_8             8U
#define PATTERN_STEPS_16            16U
#define PATTERN_STEPS_24            24U
#define PATTERN_STEPS_32            32U

#endif