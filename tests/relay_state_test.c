#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "kitchen_led.h"

static uint32_t levels[17];
static bool fail_write;
static bool fail_indicator;

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

void ets_delay_us(uint32_t delay)
{
    (void)delay;
}

void ws2812_write_parallel(const uint32_t *masks, size_t bits, uint32_t pins)
{
    assert(bits == 50 * 24);
    assert((pins & (1U << KITCHEN_RELAY_GPIO)) == 0);
    assert((pins & (1U << BLUE_LED)) == 0);
    for (size_t bit = 0; bit < bits; ++bit) {
        assert(masks[bit] == pins);
    }
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
#endif
    return 0;
}
