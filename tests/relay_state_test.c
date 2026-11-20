#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "kitchen_led.h"
#include "sdkconfig.h"

static uint32_t levels[17];
static bool fail_write;
static bool fail_indicator;
static bool colored_frame;
static bool second_line_frame;

esp_err_t gpio_set_level(unsigned pin, uint32_t level)
{
    if (pin == KITCHEN_RELAY_GPIO && fail_write) {
        return ESP_FAIL;
    }
    if (pin == BLUE_LED && fail_indicator) {
        return ESP_FAIL;
    }
    levels[pin] = level;
    return ESP_OK;
}

esp_err_t gpio_config(const gpio_config_t *config)
{
    if (config->pin_bit_mask == (1ULL << KITCHEN_RELAY_GPIO)) {
        assert(levels[KITCHEN_RELAY_GPIO] == 0);
        assert(config->pull_down_en == GPIO_PULLDOWN_DISABLE);
    }
    if (config->pin_bit_mask == (1ULL << BLUE_LED)) {
        assert(levels[BLUE_LED] == 1);
        assert(config->pull_down_en == GPIO_PULLDOWN_DISABLE);
    }
    return ESP_OK;
}

static bool radio_deferred;

void vPortETSIntrLock(void)
{
    assert(!radio_deferred);
    radio_deferred = true;
}

void vPortETSIntrUnlock(void)
{
    assert(radio_deferred);
    radio_deferred = false;
}

void ets_delay_us(uint32_t delay)
{
    assert(!radio_deferred);
    (void)delay;
}

static unsigned writes;
static unsigned late_attempts;
uint32_t ws2812_write_parallel(const uint32_t *masks, size_t bits, uint32_t pins)
{
    assert(radio_deferred);
#ifdef TEST_LONG_STRIP
    assert(bits == 144 * 24);
#else
    assert(bits == 50 * 24);
#endif
    assert((pins & (1U << KITCHEN_RELAY_GPIO)) == 0);
    assert((pins & (1U << BLUE_LED)) == 0);
#ifdef CONFIG_KITCHEN_LED_COLOR_ORDER_RGB
    const unsigned red_bit = 0;
    const unsigned green_bit = 15;
#else
    const unsigned red_bit = 8;
    const unsigned green_bit = 7;
#endif
    for (size_t bit = 0; bit < bits; ++bit) {
        uint32_t expected = pins;
        if (colored_frame && (bit == 143 * 24 + red_bit ||
                              bit == 143 * 24 + green_bit || bit == 143 * 24 + 22)) {
            expected &= ~(1U << CONFIG_KITCHEN_LINE_1_GPIO);
        }
        if (second_line_frame && (bit == green_bit - 7 || bit == red_bit + 7 || bit == 22)) {
            expected &= ~(1U << CONFIG_KITCHEN_LINE_2_GPIO);
        }
        assert(masks[bit] == expected);
    }
    ++writes;
    if (late_attempts) {
        --late_attempts;
        return 48;
    }
    return 27;
}

int main(void)
{
#ifdef TEST_PIN_CONFLICT
    assert(kitchen_led_init() == ESP_ERR_INVALID_ARG);
    assert(!kitchen_relay_is_on());
    puts("Reserved pin tests passed.");
#else
    assert(KITCHEN_RELAY_GPIO == 14);
    assert(BLUE_LED == 16);
    assert(!kitchen_relay_is_on());
    assert(kitchen_relay_set(true) == ESP_ERR_INVALID_STATE);
    levels[KITCHEN_RELAY_GPIO] = 1;
    levels[BLUE_LED] = 0;
    assert(kitchen_led_init() == ESP_OK);
    assert(writes == 1 && kitchen_led_late_frames() == 0);
    late_attempts = 1;
    assert(kitchen_led_show() == ESP_OK);
    assert(writes == 3 && kitchen_led_late_frames() == 1);
    late_attempts = 2;
    assert(kitchen_led_show() == ESP_OK);
    assert(writes == 5 && kitchen_led_late_frames() == 2);
    late_attempts = 2;
    assert(kitchen_led_blackout() == ESP_OK && writes == 7);
    assert(kitchen_led_blackout() == ESP_OK && writes == 8);
    assert(kitchen_led_blackout() == ESP_OK && writes == 8);
    assert(kitchen_led_show() == ESP_OK && writes == 9);
    assert(kitchen_led_blackout() == ESP_OK && writes == 10);
    assert(!kitchen_relay_is_on());
    assert(levels[KITCHEN_RELAY_GPIO] == 0);
    assert(levels[BLUE_LED] == 1);

    assert(kitchen_relay_set(true) == ESP_OK);
    assert(kitchen_relay_is_on());
    assert(levels[KITCHEN_RELAY_GPIO] == 1);
    assert(levels[BLUE_LED] == 0);
    assert(kitchen_relay_set(true) == ESP_OK);
    assert(kitchen_relay_is_on());
    assert(kitchen_led_init() == ESP_ERR_INVALID_STATE);
    assert(kitchen_relay_is_on());

    fail_write = true;
    assert(kitchen_relay_set(false) == ESP_FAIL);
    assert(kitchen_relay_is_on());
    assert(levels[KITCHEN_RELAY_GPIO] == 1);
    assert(levels[BLUE_LED] == 0);
    fail_write = false;
    assert(kitchen_relay_set(false) == ESP_OK);
    assert(!kitchen_relay_is_on());
    assert(levels[KITCHEN_RELAY_GPIO] == 0);
    assert(levels[BLUE_LED] == 1);

    fail_indicator = true;
    assert(kitchen_relay_set(true) == ESP_FAIL);
    assert(kitchen_relay_is_on());
    assert(levels[KITCHEN_RELAY_GPIO] == 1);
    fail_indicator = false;
    assert(kitchen_relay_set(true) == ESP_OK);
    assert(levels[BLUE_LED] == 0);
    assert(kitchen_relay_set(false) == ESP_OK);
    assert(!kitchen_relay_is_on());
    assert(levels[BLUE_LED] == 1);
    puts("Relay state tests passed.");
#ifdef TEST_LONG_STRIP
    assert(kitchen_led_set_pixel(0, 144, 1, 1, 1) == ESP_ERR_INVALID_ARG);
    assert(kitchen_led_set_pixel(4, 0, 1, 1, 1) == ESP_ERR_INVALID_ARG);
    assert(kitchen_led_set_pixel(0, 143, 0x80, 0x01, 0x02) == ESP_OK);
    colored_frame = true;
    assert(kitchen_led_show() == ESP_OK);
    colored_frame = false;
    assert(kitchen_led_blackout() == ESP_OK);
    second_line_frame = true;
    assert(kitchen_led_set_pixel(1, 0, 0x80, 0x01, 0x02) == ESP_OK);
    assert(kitchen_led_show() == ESP_OK);
    puts("144-pixel frame and boundary tests passed.");
#endif
#endif
    return 0;
}
