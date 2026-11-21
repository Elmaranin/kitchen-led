#include "ota_server.h"
#include "ota_secret.h"
#include "kitchen_led.h"
#include "relay_command.h"
#include "effect_control.h"
#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include "esp_http_server.h"
#include "esp_image_format.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

extern const char update_html_start[] asm("_binary_update_html_start");

static const char *TAG = "ota";
static httpd_handle_t server;
static esp_timer_handle_t restart_timer;
static bool reboot_pending;
static uint32_t boot_id;
static char upload_buffer[1024];

static int receive_upload(httpd_handle_t handle, int socket, char *buffer, size_t length, int flags)
{
    int count = recv(socket, buffer, length, flags);
    if (count >= 0) {
        return count;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        return HTTPD_SOCK_ERR_TIMEOUT;
    }
    return HTTPD_SOCK_ERR_FAIL;
}

static esp_err_t reject(httpd_req_t *request, const char *status, const char *message)
{
    httpd_resp_set_status(request, status);
    httpd_resp_set_type(request, "text/plain");
    httpd_resp_set_hdr(request, "Connection", "close");
    httpd_resp_send(request, message, strlen(message));
    return ESP_FAIL;
}

static void restart_device(void *arg)
{
    esp_restart();
}

static esp_err_t page_handler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "text/html");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, update_html_start, strlen(update_html_start));
}

static esp_err_t status_handler(httpd_req_t *request)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_app_desc_t *description = esp_ota_get_app_description();
    if (running == NULL) {
        return reject(request, "500 Internal Server Error", "The running firmware slot was not found.");
    }

    effect_settings_t effect = effect_settings_get();
#ifdef CONFIG_KITCHEN_TEST_EFFECT
    const char *effect_enabled = "true";
#else
    const char *effect_enabled = "false";
#endif
    char response[384];
    snprintf(response, sizeof(response),
             "{\"version\":\"%.31s\",\"slot\":\"%.15s\",\"free_heap\":%u,\"reboot_pending\":%s,\"relay_on\":%s,"
             "\"effect_enabled\":%s,\"speed_percent\":%u,\"brightness\":%u,\"led_late_frames\":%u,\"boot_id\":\"%08x\",\"define_line\":%u,\"test_color\":%u,\"effect\":%u}",
             description->version, running->label, (unsigned)esp_get_free_heap_size(),
             reboot_pending ? "true" : "false", kitchen_relay_is_on() ? "true" : "false",
             effect_enabled, effect.speed_percent, effect.brightness,
             (unsigned)kitchen_led_late_frames(), (unsigned)boot_id, effect.define_line, effect.test_color, effect.effect);
    httpd_resp_set_type(request, "application/json");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, response, strlen(response));
}

static esp_err_t relay_handler(httpd_req_t *request)
{
    if (reboot_pending) {
        return reject(request, "409 Conflict", "The controller is restarting after a firmware update.");
    }
    char content_type[48];
    if (httpd_req_get_hdr_value_str(request, "Content-Type", content_type, sizeof(content_type)) != ESP_OK ||
        strncasecmp(content_type, "application/json", 16) != 0 ||
        (content_type[16] != '\0' && content_type[16] != ';')) {
        return reject(request, "415 Unsupported Media Type", "Relay commands require application/json.");
    }
    char body[16];
    if (request->content_len == 0 || request->content_len > sizeof(body)) {
        return reject(request, "400 Bad Request", "Send true or false to set the relay state.");
    }
    size_t received = 0;
    while (received < request->content_len) {
        int count = httpd_req_recv(request, body + received, request->content_len - received);
        if (count <= 0) {
            return reject(request, "400 Bad Request", "The relay command was incomplete.");
        }
        received += count;
    }
    bool on;
    if (!relay_command_parse(body, received, &on)) {
        return reject(request, "400 Bad Request", "Send true or false to set the relay state.");
    }
    if (kitchen_relay_set(on) != ESP_OK) {
        return reject(request, "500 Internal Server Error", "The relay state could not be changed.");
    }
    const char *response = kitchen_relay_is_on() ? "{\"on\":true}" : "{\"on\":false}";
    httpd_resp_set_type(request, "application/json");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, response, strlen(response));
}

static esp_err_t effect_handler(httpd_req_t *request)
{
    if (reboot_pending) {
        return reject(request, "409 Conflict", "The controller is restarting after a firmware update.");
    }
#ifndef CONFIG_KITCHEN_TEST_EFFECT
    return reject(request, "409 Conflict", "LED effects are disabled in this firmware.");
#else
    char content_type[48];
    if (httpd_req_get_hdr_value_str(request, "Content-Type", content_type, sizeof(content_type)) != ESP_OK ||
        strncasecmp(content_type, "application/json", 16) != 0 ||
        (content_type[16] != '\0' && content_type[16] != ';')) {
        return reject(request, "415 Unsupported Media Type", "Effect commands require application/json.");
    }
    char body[96];
    if (request->content_len == 0 || request->content_len >= sizeof(body)) {
        return reject(request, "400 Bad Request", "The effect command is empty or too large.");
    }
    size_t received = 0;
    while (received < request->content_len) {
        int count = httpd_req_recv(request, body + received, request->content_len - received);
        if (count <= 0) {
            return reject(request, "400 Bad Request", "The effect command was incomplete.");
        }
        received += count;
    }
    effect_settings_t settings;
    if (!effect_settings_parse(body, received, &settings) || !effect_settings_set(settings)) {
        return reject(request, "400 Bad Request", "Send integer speed_percent (25 to 2000) and brightness (0 to 100), with optional define_line (0 to 4) and test_color (0 to 7), and effect (0 to 7).");
    }
    char response[112];
    snprintf(response, sizeof(response), "{\"speed_percent\":%u,\"brightness\":%u,\"define_line\":%u,\"test_color\":%u,\"effect\":%u}",
             settings.speed_percent, settings.brightness, settings.define_line, settings.test_color, settings.effect);
    httpd_resp_set_type(request, "application/json");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, response, strlen(response));
#endif
}

static bool authorized(httpd_req_t *request)
{
    static const char expected[] = "Bearer " KITCHEN_OTA_TOKEN;
    char supplied[sizeof(expected)];
    if (httpd_req_get_hdr_value_len(request, "Authorization") != sizeof(expected) - 1 ||
        httpd_req_get_hdr_value_str(request, "Authorization", supplied, sizeof(supplied)) != ESP_OK) {
        return false;
    }
    unsigned difference = 0;
    for (size_t i = 0; i < sizeof(expected) - 1; ++i) {
        difference |= (unsigned char)supplied[i] ^ (unsigned char)expected[i];
    }
    return difference == 0;
}

static esp_err_t reboot_handler(httpd_req_t *request)
{
    if (!authorized(request)) {
        return reject(request, "401 Unauthorized", "The OTA access token is missing or invalid.");
    }
    if (reboot_pending) {
        return reject(request, "409 Conflict", "The controller is already restarting.");
    }
    if (request->content_len != 0) {
        return reject(request, "400 Bad Request", "The reboot request must have an empty body.");
    }
    reboot_pending = true;
    if (esp_timer_start_once(restart_timer, 1000000) != ESP_OK) {
        reboot_pending = false;
        return reject(request, "500 Internal Server Error", "The restart could not be scheduled.");
    }
    httpd_resp_set_type(request, "application/json");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "Connection", "close");
    const char response[] = "{\"reboot_pending\":true}";
    return httpd_resp_send(request, response, sizeof(response) - 1);
}

static esp_err_t read_app_description(const esp_partition_t *partition,
                                      const esp_image_metadata_t *metadata,
                                      esp_app_desc_t *description)
{
    /* The ESP8266 linker places the descriptor at the start of its read-only data segment. */
    for (unsigned i = 0; i < metadata->image.segment_count; ++i) {
        if (metadata->segments[i].data_len < sizeof(*description)) {
            continue;
        }
        esp_err_t err = esp_partition_read(partition,
                                           metadata->segment_data[i] - partition->address,
                                           description, sizeof(*description));
        if (err != ESP_OK) {
            return err;
        }
        if (description->magic_word == ESP_APP_DESC_MAGIC_WORD) {
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t upload_handler(httpd_req_t *request)
{
    if (!authorized(request)) {
        return reject(request, "401 Unauthorized", "The OTA access token is missing or invalid.");
    }
    if (reboot_pending) {
        return reject(request, "409 Conflict", "A firmware update is already waiting for reboot.");
    }

    const esp_partition_t *target = esp_ota_get_next_update_partition(NULL);
    if (target == NULL) {
        return reject(request, "500 Internal Server Error", "An inactive OTA slot was not found.");
    }
    if (request->content_len == 0 || request->content_len > target->size) {
        return reject(request, "413 Payload Too Large", "The firmware size is empty or exceeds the OTA slot.");
    }

    esp_err_t err = httpd_sess_set_recv_override(request->handle,
                                                httpd_req_to_sockfd(request), receive_upload);
    if (err != ESP_OK) {
        return reject(request, "500 Internal Server Error", "The upload receiver could not be initialized.");
    }

    ESP_LOGI(TAG, "Preparing %s for %u firmware bytes.", target->label, (unsigned)request->content_len);
    esp_ota_handle_t handle;
    err = esp_ota_begin(target, request->content_len, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Cannot begin update: %s", esp_err_to_name(err));
        return reject(request, "500 Internal Server Error", "The OTA slot could not be prepared.");
    }

    ESP_LOGI(TAG, "Receiving %u firmware bytes into %s.", (unsigned)request->content_len, target->label);
    size_t received = 0;
    bool write_failed = false;
    int64_t deadline = esp_timer_get_time() + 120000000;
    while (received < request->content_len) {
        size_t remaining = request->content_len - received;
        size_t wanted = remaining < sizeof(upload_buffer) ? remaining : sizeof(upload_buffer);
        int count = httpd_req_recv(request, upload_buffer, wanted);
        if (count == HTTPD_SOCK_ERR_TIMEOUT && esp_timer_get_time() <= deadline) {
            continue;
        }
        if (count <= 0 || esp_timer_get_time() > deadline) {
            ESP_LOGW(TAG, "Upload stopped after %u of %u bytes.",
                     (unsigned)received, (unsigned)request->content_len);
            /* This SDK releases an unfinished OTA handle through esp_ota_end(). */
            esp_ota_end(handle);
            return reject(request, "408 Request Timeout", "The upload was interrupted. The running firmware is unchanged.");
        }
        if (!write_failed) {
            err = esp_ota_write(handle, upload_buffer, count);
            write_failed = err != ESP_OK;
        }
        received += count;
        if (received % 65536 < (size_t)count) {
            ESP_LOGI(TAG, "Received %u of %u bytes.", (unsigned)received,
                     (unsigned)request->content_len);
        }
        vTaskDelay(1);
    }

    err = esp_ota_end(handle);
    if (write_failed) {
        return reject(request, "400 Bad Request", "The firmware could not be written or its image header is invalid.");
    }
    if (err != ESP_OK) {
        return reject(request, "400 Bad Request", "Firmware validation failed. The running firmware is unchanged.");
    }

    esp_image_metadata_t metadata;
    const esp_partition_pos_t uploaded = {
        .offset = target->address,
        .size = request->content_len,
    };
    if (esp_image_load(ESP_IMAGE_VERIFY, &uploaded, &metadata) != ESP_OK ||
        metadata.image_len > received) {
        return reject(request, "400 Bad Request", "The uploaded firmware image is incomplete.");
    }

    esp_app_desc_t description;
    err = read_app_description(target, &metadata, &description);
    if (err != ESP_OK ||
        strncmp(description.project_name, esp_ota_get_app_description()->project_name,
                sizeof(description.project_name)) != 0) {
        return reject(request, "400 Bad Request", "The firmware does not belong to this project.");
    }
    for (size_t i = 0; i < sizeof(description.version); ++i) {
        char c = description.version[i];
        if (c == '\0') {
            break;
        }
        if ((c < '0' || c > '9') && c != '.') {
            return reject(request, "400 Bad Request", "The firmware version has an invalid format.");
        }
    }

    err = esp_ota_set_boot_partition(target);
    if (err != ESP_OK) {
        return reject(request, "500 Internal Server Error", "The new firmware could not be selected for boot.");
    }
    reboot_pending = true;
    ESP_LOGI(TAG, "Firmware validated. Rebooting into %s.", target->label);
    httpd_resp_set_type(request, "text/plain");
    httpd_resp_set_hdr(request, "Connection", "close");
    const char response[] = "Firmware accepted. The controller will reboot now.\n";
    esp_err_t sent = httpd_resp_send(request, response, sizeof(response) - 1);
    ESP_ERROR_CHECK(esp_timer_start_once(restart_timer, 1000000));
    return sent;
}

esp_err_t ota_server_start(void)
{
    if (server != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_timer_create_args_t timer_config = {
        .callback = restart_device,
        .name = "ota_restart",
    };
    esp_err_t err = esp_timer_create(&timer_config, &restart_timer);
    if (err != ESP_OK) {
        return err;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    boot_id = esp_random();
    config.stack_size = 8192;
    config.max_open_sockets = 3;
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 10;
    err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        return err;
    }
    const httpd_uri_t handlers[] = {
        {.uri = "/", .method = HTTP_GET, .handler = page_handler},
        {.uri = "/status", .method = HTTP_GET, .handler = status_handler},
        {.uri = "/relay", .method = HTTP_POST, .handler = relay_handler},
        {.uri = "/effect", .method = HTTP_POST, .handler = effect_handler},
        {.uri = "/update", .method = HTTP_POST, .handler = upload_handler},
        {.uri = "/reboot", .method = HTTP_POST, .handler = reboot_handler},
    };
    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); ++i) {
        err = httpd_register_uri_handler(server, &handlers[i]);
        if (err != ESP_OK) {
            return err;
        }
    }

    ESP_LOGI(TAG, "Controller and OTA tabs are ready on port 80. Firmware version: %s.",
             esp_ota_get_app_description()->version);
    return ESP_OK;
}
