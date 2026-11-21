#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "led_layout.h"

typedef enum {
    EFFECT_IDENTIFY,
    EFFECT_RAINBOW,
    EFFECT_AURORA,
    EFFECT_COMETS,
    EFFECT_EMBERS,
    EFFECT_TWINKLE,
    EFFECT_BREATHE,
    EFFECT_SOLID,
    EFFECT_COUNT,
} led_effect_t;

typedef struct {
    unsigned speed_percent;
    unsigned brightness;
    unsigned define_line;
    unsigned test_color;
    unsigned effect;
} effect_settings_t;

effect_settings_t effect_settings_get(void);
bool effect_settings_set(effect_settings_t settings);
bool effect_settings_parse(const char *body, size_t length, effect_settings_t *settings);
unsigned effect_frame_ms(effect_settings_t settings);
uint64_t effect_pixel_steps(uint64_t elapsed_us, unsigned speed_percent, unsigned *remainder);
uint8_t effect_peak(effect_settings_t settings);
void effect_rgb(effect_settings_t settings, unsigned line, uint8_t rgb[3]);
