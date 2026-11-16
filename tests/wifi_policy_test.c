#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "wifi_policy.h"

int main(void)
{
    wifi_policy_t policy;
    wifi_policy_init(&policy, 100);
    assert(wifi_policy_step(&policy, 100, false, true, 30000) == WIFI_POLICY_CONNECT);
    assert(wifi_policy_step(&policy, 200, false, true, 30000) == WIFI_POLICY_WAIT);
    assert(wifi_policy_step(&policy, 2100, false, false, 30000) == WIFI_POLICY_WAIT);
    assert(wifi_policy_step(&policy, 2100, false, true, 30000) == WIFI_POLICY_CONNECT);
    assert(wifi_policy_step(&policy, 30099, false, false, 30000) == WIFI_POLICY_WAIT);
    assert(wifi_policy_step(&policy, 30100, false, false, 30000) == WIFI_POLICY_START_AP);

    wifi_policy_init(&policy, 0);
    assert(wifi_policy_step(&policy, 1000, true, false, 30000) == WIFI_POLICY_WAIT);
    assert(wifi_policy_step(&policy, 100000, true, true, 30000) == WIFI_POLICY_WAIT);
    assert(wifi_policy_step(&policy, 100250, false, true, 30000) == WIFI_POLICY_CONNECT);
    assert(wifi_policy_step(&policy, 130249, false, false, 30000) == WIFI_POLICY_WAIT);
    assert(wifi_policy_step(&policy, 130250, false, false, 30000) == WIFI_POLICY_START_AP);

    wifi_policy_init(&policy, 0);
    assert(wifi_policy_step(&policy, 29000, true, false, 30000) == WIFI_POLICY_WAIT);
    assert(wifi_policy_step(&policy, 29500, false, true, 30000) == WIFI_POLICY_CONNECT);
    assert(wifi_policy_step(&policy, 30000, true, false, 30000) == WIFI_POLICY_WAIT);
    assert(wifi_policy_step(&policy, 60000, true, false, 30000) == WIFI_POLICY_WAIT);
    assert(wifi_policy_step(&policy, 61000, false, true, 30000) == WIFI_POLICY_CONNECT);
    assert(wifi_policy_step(&policy, 91000, false, false, 30000) == WIFI_POLICY_START_AP);

    wifi_policy_init(&policy, UINT32_MAX - 1000);
    assert(wifi_policy_step(&policy, UINT32_MAX - 1000, false, true, 30000) == WIFI_POLICY_CONNECT);
    assert(wifi_policy_step(&policy, 999, false, true, 30000) == WIFI_POLICY_CONNECT);
    assert(wifi_policy_step(&policy, 28998, false, false, 30000) == WIFI_POLICY_WAIT);
    assert(wifi_policy_step(&policy, 28999, false, false, 30000) == WIFI_POLICY_START_AP);

    puts("Wi-Fi policy tests passed.");
    return 0;
}
