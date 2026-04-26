#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#define FOSC_HZ                     40000000UL
#define FCY_HZ                      (FOSC_HZ / 4UL)
#define AUDIO_RATE_HZ               25000UL
#define TMR0_RELOAD                 (65536UL - (FCY_HZ / AUDIO_RATE_HZ))
#define STEPS_MIN                   8U
#define STEPS_MAX                   32U
#define AUDIO_SAMPLE_MAX            255
#define AUDIO_SAMPLE_MIN            (-256)
#define UI_UPDATE_DIVIDER_TICKS     250U

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

#endif