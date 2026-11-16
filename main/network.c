#include "network.h"
#include "wifi_policy.h"

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "mdns.h"

#ifndef CONFIG_ENABLE_MDNS
#error "Enable mDNS in the SDK component configuration to build this project."
#endif

#define CONNECTED_BIT BIT0
#define RETRY_READY_BIT BIT1

_Static_assert(sizeof(CONFIG_KITCHEN_WIFI_SSID) - 1 <= 32,
               "The station SSID must contain at most 32 bytes.");
_Static_assert(sizeof(CONFIG_KITCHEN_WIFI_PASSWORD) - 1 <= 64,
               "The station password must contain at most 64 bytes.");
_Static_assert(sizeof(CONFIG_KITCHEN_AP_PASSWORD) - 1 >= 8 &&
               sizeof(CONFIG_KITCHEN_AP_PASSWORD) - 1 <= 63,
               "The fallback AP password must contain 8 to 63 bytes.");
_Static_assert(sizeof(CONFIG_KITCHEN_MDNS_HOSTNAME) > 1 &&
               sizeof(CONFIG_KITCHEN_MDNS_HOSTNAME) <= 64,
               "The mDNS hostname must contain 1 to 63 bytes.");

static const char *TAG = "network";
static EventGroupHandle_t wifi_events;
static bool started;

static esp_err_t start_mdns(void)
{
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        return err;
    }
    err = mdns_hostname_set(CONFIG_KITCHEN_MDNS_HOSTNAME);
    if (err == ESP_OK) {
        err = mdns_instance_name_set("Kitchen LED controller");
    }
    if (err == ESP_OK) {
        mdns_txt_item_t txt[] = {{"path", "/"}};
        err = mdns_service_add(NULL, "_http", "_tcp", 80, txt, 1);
    }
    if (err != ESP_OK) {
        mdns_free();
        return err;
    }
    ESP_LOGI(TAG, "mDNS configured: http://%s.local/", CONFIG_KITCHEN_MDNS_HOSTNAME);
    return ESP_OK;
}

static void network_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if ((base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) ||
        (base == IP_EVENT && id == IP_EVENT_STA_LOST_IP)) {
        xEventGroupClearBits(wifi_events, CONNECTED_BIT);
        xEventGroupSetBits(wifi_events, RETRY_READY_BIT);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = data;
        xEventGroupClearBits(wifi_events, RETRY_READY_BIT);
        xEventGroupSetBits(wifi_events, CONNECTED_BIT);
        ESP_LOGI(TAG, "Station connected. IP: %s", ip4addr_ntoa(&event->ip_info.ip));
    }
}

static esp_err_t start_ap(bool stop_station)
{
    uint8_t mac[6];
    esp_err_t err = esp_wifi_get_mac(ESP_IF_WIFI_AP, mac);
    if (err != ESP_OK) {
        return err;
    }

    wifi_config_t config = {
        .ap = {
            .password = CONFIG_KITCHEN_AP_PASSWORD,
            .channel = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 4,
        },
    };
    snprintf((char *)config.ap.ssid, sizeof(config.ap.ssid),
             "kitchen-led-%02X%02X%02X", mac[3], mac[4], mac[5]);
    config.ap.ssid_len = strlen((char *)config.ap.ssid);

    if (stop_station) {
        err = esp_wifi_stop();
        if (err != ESP_OK) {
            return err;
        }
    }
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_set_config(ESP_IF_WIFI_AP, &config);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        return err;
    }

    tcpip_adapter_ip_info_t info;
    err = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &info);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Fallback AP: %s. IP: %s", config.ap.ssid,
                 ip4addr_ntoa(&info.ip));
    }
    return err;
}

static void connection_task(void *arg)
{
    wifi_policy_t policy;
    wifi_policy_init(&policy, (uint32_t)(esp_timer_get_time() / 1000));

    for (;;) {
        EventBits_t bits = xEventGroupGetBits(wifi_events);
        wifi_policy_action_t action = wifi_policy_step(
            &policy, (uint32_t)(esp_timer_get_time() / 1000),
            (bits & CONNECTED_BIT) != 0, (bits & RETRY_READY_BIT) != 0,
            CONFIG_KITCHEN_WIFI_TIMEOUT_SECONDS * 1000U);

        if (action == WIFI_POLICY_START_AP) {
            ESP_LOGW(TAG, "Station connection timed out. Starting fallback AP.");
            ESP_ERROR_CHECK(start_ap(true));
            vTaskDelete(NULL);
        } else if (action == WIFI_POLICY_CONNECT) {
            xEventGroupClearBits(wifi_events, RETRY_READY_BIT);
            esp_err_t err = esp_wifi_connect();
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Station connection request failed: %s", esp_err_to_name(err));
                xEventGroupSetBits(wifi_events, RETRY_READY_BIT);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

esp_err_t network_start(void)
{
    if (started) {
        return ESP_ERR_INVALID_STATE;
    }
    started = true;

    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        return err;
    }
    wifi_events = xEventGroupCreate();
    if (wifi_events == NULL) {
        return ESP_ERR_NO_MEM;
    }
    tcpip_adapter_init();
    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        return err;
    }

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&init);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                     network_event, NULL);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, network_event, NULL);
    if (err != ESP_OK) {
        return err;
    }

    err = start_mdns();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mDNS setup failed: %s. The IP address remains available.", esp_err_to_name(err));
    }

    if (CONFIG_KITCHEN_WIFI_SSID[0] == '\0') {
        ESP_LOGI(TAG, "No station SSID is configured. Starting fallback AP.");
        return start_ap(false);
    }

    wifi_config_t config = {
        .sta = {
            .ssid = CONFIG_KITCHEN_WIFI_SSID,
            .password = CONFIG_KITCHEN_WIFI_PASSWORD,
        },
    };
    if (sizeof(CONFIG_KITCHEN_WIFI_PASSWORD) > 1) {
        config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_set_config(ESP_IF_WIFI_STA, &config);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        return err;
    }

    xEventGroupSetBits(wifi_events, RETRY_READY_BIT);
    err = esp_wifi_set_ps(WIFI_PS_NONE);
    if (err != ESP_OK) {
        return err;
    }
    if (xTaskCreate(connection_task, "wifi_connection", 3072, NULL, 4, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "Station mode started. Connection timeout: %d seconds.",
             CONFIG_KITCHEN_WIFI_TIMEOUT_SECONDS);
    return ESP_OK;
}
