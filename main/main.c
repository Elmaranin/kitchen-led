#include "kitchen_led.h"
#include "network.h"
#include "ota_server.h"
#include "effect_control.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef CONFIG_KITCHEN_TEST_EFFECT
static void run_test_effect(void)
{
    static const size_t counts[KITCHEN_LED_LINES] = {
        CONFIG_KITCHEN_LINE_1_LEDS, CONFIG_KITCHEN_LINE_2_LEDS,
        CONFIG_KITCHEN_LINE_3_LEDS, CONFIG_KITCHEN_LINE_4_LEDS,
    };
    size_t heads[KITCHEN_LED_LINES] = {0};
    ESP_LOGI("kitchen_led", "Starting identification chases: line 1 green, line 2 blue, line 3 red, line 4 white.");
    TickType_t next_frame = xTaskGetTickCount();
    unsigned tick_remainder = 0;
    unsigned pixel_remainder = 0;
    unsigned previous_speed = effect_settings_get().speed_percent;
    int64_t previous_time = 0;
    for (;;) {
        effect_settings_t settings = effect_settings_get();
        /* The SDK tick interrupt updates both parts of the microsecond clock. */
        portENTER_CRITICAL();
        int64_t now = esp_timer_get_time();
        portEXIT_CRITICAL();
        uint64_t elapsed = previous_time ? now - previous_time : 0;
        uint64_t steps = effect_pixel_steps(elapsed, previous_speed, &pixel_remainder);
        previous_time = now;
        previous_speed = settings.speed_percent;
        uint8_t peak = effect_peak(settings);
        for (size_t line = 0; line < KITCHEN_LED_LINES; ++line) {
            heads[line] = (heads[line] + steps) % counts[line];
            if (!peak) continue;
            uint8_t rgb[3];
            effect_rgb(settings, line, rgb);
            ESP_ERROR_CHECK(kitchen_led_fill(line, 0, 0, 0));
            if (settings.define_line) {
                if (settings.define_line == line + 1) {
                    ESP_ERROR_CHECK(kitchen_led_fill(line,
                        rgb[0], rgb[1], rgb[2]));
                }
                continue;
            }
            for (size_t tail = 0; tail < 4 && tail <= heads[line]; ++tail) {
                ESP_ERROR_CHECK(kitchen_led_set_pixel(line, heads[line] - tail,
                    rgb[0] >> tail, rgb[1] >> tail, rgb[2] >> tail));
            }
        }
        ESP_ERROR_CHECK(peak ? kitchen_led_show() : kitchen_led_blackout());
        /* The remainder keeps the average speed accurate between timer ticks. */
        unsigned scaled_ticks = effect_frame_ms(settings) * configTICK_RATE_HZ + tick_remainder;
        TickType_t interval = scaled_ticks / 1000;
        tick_remainder = scaled_ticks % 1000;
        if (!interval) interval = 1;
        TickType_t after_show = xTaskGetTickCount();
        if (after_show - next_frame >= interval) {
            next_frame = after_show;
            interval = 1;
        }
        vTaskDelayUntil(&next_frame, interval);
    }
}
#endif

void app_main(void)
{
    ESP_ERROR_CHECK(kitchen_led_init());
    ESP_LOGI("kitchen_led", "Four LED lines are off. Relay GPIO%d is low.",
             KITCHEN_RELAY_GPIO);
    ESP_ERROR_CHECK(network_start());
    ESP_ERROR_CHECK(ota_server_start());
#ifdef CONFIG_KITCHEN_TEST_EFFECT
    run_test_effect();
#endif
}
