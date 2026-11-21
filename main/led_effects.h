#pragma once

#include "effect_control.h"

void led_effect_pixel(effect_settings_t settings, unsigned line, size_t pixel,
                      size_t count, uint64_t phase_ms, uint8_t rgb[3]);
