#include "kitchen_led.h"
#include "network.h"
#include "ota_server.h"

#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef CONFIG_KITCHEN_TEST_EFFECT
#if CONFIG_KITCHEN_LINE_2_GPIO != 5
#error "The LED test effect requires GPIO5 for line 2."
#endif

static void run_test_effect(void)
{
    ESP_LOGI("kitchen_led", "Starting the test chase on GPIO5 with %d LEDs.",
             CONFIG_KITCHEN_LINE_2_LEDS);
    unsigned color = 0;
    TickType_t next_frame = xTaskGetTickCount();
    for (;;) {
        for (size_t head = 0; head < CONFIG_KITCHEN_LINE_2_LEDS; ++head) {
            ESP_ERROR_CHECK(kitchen_led_fill(1, 0, 0, 0));
            for (size_t tail = 0; tail < 4 && tail <= head; ++tail) {
                uint8_t brightness = 16U >> tail;
                ESP_ERROR_CHECK(kitchen_led_set_pixel(1, head - tail,
                    color == 0 ? brightness : 0,
                    color == 1 ? brightness : 0,
                    color == 2 ? brightness : 0));
            }
            ESP_ERROR_CHECK(kitchen_led_show());
            vTaskDelayUntil(&next_frame, pdMS_TO_TICKS(100));
        }
        color = (color + 1) % 3;
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
