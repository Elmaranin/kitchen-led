#include "kitchen_led.h"
#include "network.h"
#include "ota_server.h"

#include "esp_log.h"

void app_main(void)
{
    ESP_ERROR_CHECK(kitchen_led_init());
    ESP_LOGI("kitchen_led", "Four LED lines are off. Relay GPIO%d is low.",
             KITCHEN_RELAY_GPIO);
    ESP_ERROR_CHECK(network_start());
    ESP_ERROR_CHECK(ota_server_start());
}
