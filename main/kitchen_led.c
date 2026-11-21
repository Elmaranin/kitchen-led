#include "kitchen_led.h"
#include <string.h>

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
    [KITCHEN_LINE_RIGHT_UP] = CONFIG_KITCHEN_LINE_1_GPIO,
    [KITCHEN_LINE_RIGHT_DOWN] = CONFIG_KITCHEN_LINE_2_GPIO,
    [KITCHEN_LINE_LEFT_DOWN] = CONFIG_KITCHEN_LINE_3_GPIO,
    [KITCHEN_LINE_LEFT_UP] = CONFIG_KITCHEN_LINE_4_GPIO,
};
static const size_t counts[KITCHEN_LED_LINES] = {
    [KITCHEN_LINE_RIGHT_UP] = CONFIG_KITCHEN_LINE_1_LEDS,
    [KITCHEN_LINE_RIGHT_DOWN] = CONFIG_KITCHEN_LINE_2_LEDS,
    [KITCHEN_LINE_LEFT_DOWN] = CONFIG_KITCHEN_LINE_3_LEDS,
    [KITCHEN_LINE_LEFT_UP] = CONFIG_KITCHEN_LINE_4_LEDS,
};
static uint8_t pixels[KITCHEN_LED_LINES][MAX_LEDS][3];
static uint32_t zero_masks[MAX_LEDS * 24];
static uint32_t output_mask;
static bool initialized;
static bool relay_on;

static uint32_t late_frames;
static bool holding_black;

static const uint8_t swapped_lines = 0
#ifdef CONFIG_KITCHEN_LINE_1_SWAP_RG
    | (1U << KITCHEN_LINE_RIGHT_UP)
#endif
#ifdef CONFIG_KITCHEN_LINE_2_SWAP_RG
    | (1U << KITCHEN_LINE_RIGHT_DOWN)
#endif
#ifdef CONFIG_KITCHEN_LINE_3_SWAP_RG
    | (1U << KITCHEN_LINE_LEFT_DOWN)
#endif
#ifdef CONFIG_KITCHEN_LINE_4_SWAP_RG
    | (1U << KITCHEN_LINE_LEFT_UP)
#endif
    ;

uint32_t ws2812_write_parallel(const uint32_t *masks, size_t bits, uint32_t pins);

uint32_t kitchen_led_late_frames(void)
{
    portENTER_CRITICAL();
    uint32_t count = late_frames;
    portEXIT_CRITICAL();
    return count;
}

esp_err_t kitchen_led_init(void)
{
    if (initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t mask = 0;
    for (size_t line = 0; line < KITCHEN_LED_LINES; ++line) {
        unsigned pin = pins[line];
        if (pin != 4 && pin != 5 && pin != 12 && pin != 13) {
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
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_set_level(KITCHEN_RELAY_GPIO, 0);
    if (err != ESP_OK) {
        return err;
    }
    relay_on = false;
    err = gpio_config(&relay);
    if (err != ESP_OK) {
        return err;
    }

    gpio_config_t indicator = {
        .pin_bit_mask = 1ULL << BLUE_LED,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_set_level(BLUE_LED, 1);
    if (err != ESP_OK) {
        return err;
    }
    err = gpio_config(&indicator);
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
    if (swapped_lines & (1U << line)) {
        uint8_t saved_red = red;
        red = green;
        green = saved_red;
    }
#ifdef CONFIG_KITCHEN_LED_COLOR_ORDER_RGB
    pixels[line][pixel][0] = red;
    pixels[line][pixel][1] = green;
#else
    pixels[line][pixel][0] = green;
    pixels[line][pixel][1] = red;
#endif
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

static void encode_frame(void)
{
    for (size_t pixel = 0; pixel < MAX_LEDS; ++pixel) {
        for (size_t channel = 0; channel < 3; ++channel) {
            uint32_t *masks = &zero_masks[pixel * 24 + channel * 8];
            for (unsigned bit = 0; bit < 8; ++bit) {
                masks[bit] = output_mask;
            }
            for (size_t line = 0; line < KITCHEN_LED_LINES; ++line) {
                uint8_t value = pixels[line][pixel][channel];
                if (value == 0) continue;
                uint32_t pin_mask = 1U << pins[line];
                for (unsigned bit = 0; bit < 8; ++bit) {
                    if (value & (0x80U >> bit)) masks[bit] &= ~pin_mask;
                }
            }
        }
    }

}

static bool send_frame(void)
{
    /* The low interval resets the LEDs before and after each frame. */
    ets_delay_us(300);
    for (unsigned attempt = 0; attempt < 2; ++attempt) {
        /* The SDK defers Wi-Fi MAC interrupts here and keeps the watchdog timer enabled. */
        vPortETSIntrLock();
        uint32_t peak_cycles = ws2812_write_parallel(zero_masks, MAX_LEDS * 24, output_mask);
        vPortETSIntrUnlock();
        /* At 80 MHz, 48 cycles give the CPU 600 ns to reach the zero-bit clear. */
        bool late = peak_cycles >= 48;
        if (late && attempt == 0) {
            portENTER_CRITICAL();
            ++late_frames;
            portEXIT_CRITICAL();
        }
        ets_delay_us(300);
        if (!late) return true;
    }
    return false;
}

esp_err_t kitchen_led_show(void)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    holding_black = false;
    encode_frame();
    send_frame();
    return ESP_OK;
}

esp_err_t kitchen_led_blackout(void)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    memset(pixels, 0, sizeof(pixels));
    if (!holding_black) {
        for (size_t bit = 0; bit < MAX_LEDS * 24; ++bit) {
            zero_masks[bit] = output_mask;
        }
        holding_black = send_frame();
    }
    return ESP_OK;
}

esp_err_t kitchen_relay_set(bool on)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = gpio_set_level(KITCHEN_RELAY_GPIO, on ? 1 : 0);
    if (err == ESP_OK) {
        relay_on = on;
        err = gpio_set_level(BLUE_LED, on ? 0 : 1);
    }
    return err;
}

bool kitchen_relay_is_on(void)
{
    return relay_on;
}
