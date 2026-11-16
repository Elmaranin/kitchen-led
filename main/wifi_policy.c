#include "wifi_policy.h"

void wifi_policy_init(wifi_policy_t *policy, uint32_t now_ms)
{
    *policy = (wifi_policy_t) {
        .offline_since_ms = now_ms,
        .last_attempt_ms = now_ms - WIFI_RETRY_INTERVAL_MS,
    };
}

wifi_policy_action_t wifi_policy_step(wifi_policy_t *policy, uint32_t now_ms,
                                     bool connected, bool retry_ready,
                                     uint32_t timeout_ms)
{
    if (connected) {
        policy->was_connected = true;
        return WIFI_POLICY_WAIT;
    }

    if (policy->was_connected) {
        wifi_policy_init(policy, now_ms);
    }
    if ((uint32_t)(now_ms - policy->offline_since_ms) >= timeout_ms) {
        return WIFI_POLICY_START_AP;
    }
    if (retry_ready &&
        (uint32_t)(now_ms - policy->last_attempt_ms) >= WIFI_RETRY_INTERVAL_MS) {
        policy->last_attempt_ms = now_ms;
        return WIFI_POLICY_CONNECT;
    }
    return WIFI_POLICY_WAIT;
}
