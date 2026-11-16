#include "kitchen_led.h"

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if CONFIG_ESP8266_DEFAULT_CPU_FREQ_MHZ != 80 || defined(CONFIG_PM_ENABLE)
#error "The WS2812 transmitter requires a fixed CPU frequency of 80 MHz."
#endif

#define MAX2(a, b) ((a) > (b) ? (a) : (b))
#define MAX_LEDS MAX2(MAX2(CONFIG_KITCHEN_LINE_1_LEDS, CONFIG_KITCHEN_LINE_2_LEDS), \
                      MAX2(CONFIG_KITCHEN_LINE_3_LEDS, CONFIG_KITCHEN_LINE_4_LEDS))

static const unsigned pins[KITCHEN_LED_LINES] = {
    CONFIG_KITCHEN_LINE_1_GPIO, CONFIG_KITCHEN_LINE_2_GPIO,
    CONFIG_KITCHEN_LINE_3_GPIO, CONFIG_KITCHEN_LINE_4_GPIO,
};
static const size_t counts[KITCHEN_LED_LINES] = {
    CONFIG_KITCHEN_LINE_1_LEDS, CONFIG_KITCHEN_LINE_2_LEDS,
    CONFIG_KITCHEN_LINE_3_LEDS, CONFIG_KITCHEN_LINE_4_LEDS,
};
static uint8_t pixels[KITCHEN_LED_LINES][MAX_LEDS][3];
static uint32_t zero_masks[MAX_LEDS * 24];
static uint32_t output_mask;
static bool initialized;

void ws2812_write_parallel(const uint32_t *masks, size_t bits, uint32_t pins);

esp_err_t kitchen_led_init(void)
{
    if (initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t mask = 0;
    for (size_t line = 0; line < KITCHEN_LED_LINES; ++line) {
        unsigned pin = pins[line];
        if (pin != 4 && pin != 5 && pin != 12 && pin != 13 && pin != 14) {
            return ESP_ERR_INVALID_ARG;
        }
        if (mask & (1U << pin)) {
            return ESP_ERR_INVALID_ARG;
        }
        mask |= 1U << pin;
    }
    output_mask = mask;

    gpio_config_t relay = {
        .pin_bit_mask = 1ULL << KITCHEN_RELAY_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_set_level(KITCHEN_RELAY_GPIO, 0);
    if (err != ESP_OK) {
        return err;
    }
    err = gpio_config(&relay);
    if (err != ESP_OK) {
        return err;
    }

    gpio_config_t leds = {
        .pin_bit_mask = output_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    for (size_t line = 0; line < KITCHEN_LED_LINES; ++line) {
        err = gpio_set_level(pins[line], 0);
        if (err != ESP_OK) {
            return err;
        }
    }
    err = gpio_config(&leds);
    if (err != ESP_OK) {
        return err;
    }

    initialized = true;
    return kitchen_led_show();
}

esp_err_t kitchen_led_set_pixel(size_t line, size_t pixel,
                               uint8_t red, uint8_t green, uint8_t blue)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (line >= KITCHEN_LED_LINES || pixel >= counts[line]) {
        return ESP_ERR_INVALID_ARG;
    }
    pixels[line][pixel][0] = green;
    pixels[line][pixel][1] = red;
    pixels[line][pixel][2] = blue;
    return ESP_OK;
}

esp_err_t kitchen_led_fill(size_t line, uint8_t red, uint8_t green, uint8_t blue)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (line >= KITCHEN_LED_LINES) {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t pixel = 0; pixel < counts[line]; ++pixel) {
        kitchen_led_set_pixel(line, pixel, red, green, blue);
    }
    return ESP_OK;
}

esp_err_t kitchen_led_show(void)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    for (size_t pixel = 0; pixel < MAX_LEDS; ++pixel) {
        for (size_t channel = 0; channel < 3; ++channel) {
            for (unsigned bit = 0; bit < 8; ++bit) {
                uint32_t mask = 0;
                for (size_t line = 0; line < KITCHEN_LED_LINES; ++line) {
                    if (pixel >= counts[line] ||
                        !(pixels[line][pixel][channel] & (0x80U >> bit))) {
                        mask |= 1U << pins[line];
                    }
                }
                zero_masks[pixel * 24 + channel * 8 + bit] = mask;
            }
        }
    }

    /* The low interval resets the LEDs before and after each frame. */
    ets_delay_us(300);
    portENTER_CRITICAL();
    ws2812_write_parallel(zero_masks, MAX_LEDS * 24, output_mask);
    portEXIT_CRITICAL();
    ets_delay_us(300);
    return ESP_OK;
}

esp_err_t kitchen_relay_set(bool on)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    return gpio_set_level(KITCHEN_RELAY_GPIO, on ? 1 : 0);
}
