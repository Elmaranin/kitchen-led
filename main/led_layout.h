#pragma once

#define KITCHEN_LED_LINES 4

/* These zero-based indices preserve the original GPIO and HTTP line assignments. */
/* The left strips start at their right end. The right strips start at their left end. */
typedef enum {
    KITCHEN_LINE_RIGHT_UP = 0,
    KITCHEN_LINE_RIGHT_DOWN = 1,
    KITCHEN_LINE_LEFT_DOWN = 2,
    KITCHEN_LINE_LEFT_UP = 3,
} kitchen_led_line_t;
