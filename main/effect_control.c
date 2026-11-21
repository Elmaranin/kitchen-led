#include "effect_control.h"

#include <string.h>

#include "cJSON.h"
#include "freertos/FreeRTOS.h"

static effect_settings_t current = {.speed_percent = 100, .brightness = 6, .effect = EFFECT_RAINBOW};

effect_settings_t effect_settings_get(void)
{
    portENTER_CRITICAL();
    effect_settings_t settings = current;
    portEXIT_CRITICAL();
    return settings;
}

bool effect_settings_set(effect_settings_t settings)
{
    if (settings.speed_percent < 25 || settings.speed_percent > 2000 ||
        settings.brightness > 100 || settings.define_line > 4 ||
        settings.test_color > 7 || settings.effect >= EFFECT_COUNT) {
        return false;
    }
    portENTER_CRITICAL();
    current = settings;
    portEXIT_CRITICAL();
    return true;
}

bool effect_settings_parse(const char *body, size_t length, effect_settings_t *settings)
{
    char buffer[96];
    if (body == NULL || settings == NULL || length == 0 || length >= sizeof(buffer) ||
        memchr(body, '\0', length) != NULL) {
        return false;
    }
    memcpy(buffer, body, length);
    buffer[length] = '\0';
    const char *end;
    cJSON *json = cJSON_ParseWithOpts(buffer, &end, true);
    const cJSON *speed = cJSON_GetObjectItemCaseSensitive(json, "speed_percent");
    const cJSON *brightness = cJSON_GetObjectItemCaseSensitive(json, "brightness");
    const cJSON *line = cJSON_GetObjectItemCaseSensitive(json, "define_line");
    const cJSON *color = cJSON_GetObjectItemCaseSensitive(json, "test_color");
    const cJSON *effect = cJSON_GetObjectItemCaseSensitive(json, "effect");
    int fields = cJSON_GetArraySize(json);
    bool valid_line = line == NULL || (cJSON_IsNumber(line) && line->valuedouble >= 0 &&
        line->valuedouble <= 4 && line->valuedouble == line->valueint);
    bool valid_color = color == NULL || (cJSON_IsNumber(color) && color->valuedouble >= 0 &&
        color->valuedouble <= 7 && color->valuedouble == color->valueint);
    bool valid_effect = effect == NULL || (cJSON_IsNumber(effect) && effect->valuedouble >= 0 &&
        effect->valuedouble < EFFECT_COUNT && effect->valuedouble == effect->valueint);
    bool valid = cJSON_IsObject(json) && valid_line && valid_color && valid_effect &&
        fields == 2 + (line != NULL) + (color != NULL) + (effect != NULL) &&
        cJSON_IsNumber(speed) && speed->valuedouble >= 25 && speed->valuedouble <= 2000 &&
        speed->valuedouble == speed->valueint &&
        cJSON_IsNumber(brightness) && brightness->valuedouble >= 0 && brightness->valuedouble <= 100 &&
        brightness->valuedouble == brightness->valueint;
    if (valid) {
        *settings = effect_settings_get();
        if (line) settings->define_line = (unsigned)line->valueint;
        if (color) settings->test_color = (unsigned)color->valueint;
        if (effect) settings->effect = (unsigned)effect->valueint;
        settings->speed_percent = (unsigned)speed->valueint;
        settings->brightness = (unsigned)brightness->valueint;
    }
    cJSON_Delete(json);
    return valid;
}

unsigned effect_frame_ms(effect_settings_t settings)
{
    if (settings.effect != EFFECT_IDENTIFY) return 20;
    unsigned period_ms = (10000 + settings.speed_percent / 2) / settings.speed_percent;
    /* Wi-Fi needs time between transmissions even at the fastest chase speed. */
    return period_ms < 20 ? 20 : period_ms;
}

uint8_t effect_peak(effect_settings_t settings)
{
    return (uint8_t)((settings.brightness * 255 + 50) / 100);
}

uint64_t effect_pixel_steps(uint64_t elapsed_us, unsigned speed_percent, unsigned *remainder)
{
    /* At 100% speed, one pixel takes 100,000 microseconds. */
    const unsigned units_per_pixel = 100000U * 100U;
    uint64_t units = elapsed_us * speed_percent + *remainder;
    *remainder = (unsigned)(units % units_per_pixel);
    return units / units_per_pixel;
}

void effect_rgb(effect_settings_t settings, unsigned line, uint8_t rgb[3])
{
    static const uint8_t palette[8][3] = {
        {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
        {1, 1, 1}, {1, 1, 0}, {0, 1, 1}, {1, 0, 1},
    };
    static const unsigned line_colors[KITCHEN_LED_LINES] = {
        [KITCHEN_LINE_RIGHT_UP] = 2,
        [KITCHEN_LINE_RIGHT_DOWN] = 3,
        [KITCHEN_LINE_LEFT_DOWN] = 1,
        [KITCHEN_LINE_LEFT_UP] = 4,
    };
    unsigned color = settings.test_color ? settings.test_color : line_colors[line];
    uint8_t peak = effect_peak(settings);
    for (unsigned channel = 0; channel < 3; ++channel) {
        rgb[channel] = palette[color][channel] * peak;
    }
}
