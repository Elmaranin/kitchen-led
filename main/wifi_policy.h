#pragma once

#include <stdbool.h>
#include <stdint.h>

#define WIFI_RETRY_INTERVAL_MS 2000U

typedef enum {
    WIFI_POLICY_WAIT,
    WIFI_POLICY_CONNECT,
    WIFI_POLICY_START_AP,
} wifi_policy_action_t;

typedef struct {
    bool was_connected;
    uint32_t offline_since_ms;
    uint32_t last_attempt_ms;
} wifi_policy_t;

void wifi_policy_init(wifi_policy_t *policy, uint32_t now_ms);
wifi_policy_action_t wifi_policy_step(wifi_policy_t *policy, uint32_t now_ms,
                                     bool connected, bool retry_ready,
                                     uint32_t timeout_ms);
