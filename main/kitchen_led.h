#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#define KITCHEN_LED_LINES 4
#define KITCHEN_RELAY_GPIO 16

/* The caller must use one task. Line and pixel indices start at zero. */
esp_err_t kitchen_led_init(void);
esp_err_t kitchen_led_set_pixel(size_t line, size_t pixel,
                               uint8_t red, uint8_t green, uint8_t blue);
esp_err_t kitchen_led_fill(size_t line, uint8_t red, uint8_t green, uint8_t blue);
esp_err_t kitchen_led_show(void);
esp_err_t kitchen_relay_set(bool on);
