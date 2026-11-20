#include "effect_control.h"

#include <assert.h>
#include <string.h>

static void reject_command(const char *body)
{
    effect_settings_t settings = {100, 6, 0, 0};
    assert(!effect_settings_parse(body, strlen(body), &settings));
    assert(settings.speed_percent == 100 && settings.brightness == 6);
}

int main(void)
{
    /* A slow renderer still moves the head 144 pixels in 1.44 seconds at 10×. */
    unsigned remainder = 0;
    unsigned steps = 0;
    for (unsigned i = 0; i < 48; ++i) {
        steps += effect_pixel_steps(30000, 1000, &remainder);
    }
    assert(steps == 144 && remainder == 0);
    assert(effect_pixel_steps(30000, 400, &remainder) == 1);
    assert(effect_pixel_steps(8000, 1000, &remainder) == 1 && remainder == 0);
    assert(effect_pixel_steps(20000, 2000, &remainder) == 4 && remainder == 0);
    effect_settings_t settings = effect_settings_get();
    assert(settings.speed_percent == 100 && settings.brightness == 6);
    assert(effect_frame_ms(settings) == 100);
    assert(effect_peak(settings) == 15);

    const char *valid = " {\"brightness\":100,\"speed_percent\":2000} \n";
    assert(effect_settings_parse(valid, strlen(valid), &settings));
    assert(effect_settings_set(settings));
    assert(effect_settings_get().speed_percent == 2000);
    assert(effect_peak(settings) == 255);
    assert(effect_frame_ms(settings) == 20);

    const char *black = "{\"speed_percent\":1000,\"brightness\":0}";
    assert(effect_settings_parse(black, strlen(black), &settings));
    assert(effect_settings_set(settings));
    assert(effect_peak(effect_settings_get()) == 0);
    assert(effect_frame_ms(effect_settings_get()) == 20);

    settings = (effect_settings_t){25, 0, 0, 0};
    assert(effect_settings_set(settings));
    assert(effect_peak(settings) == 0);
    assert(effect_frame_ms(settings) == 400);
    assert(!effect_settings_set((effect_settings_t){0, 0, 0, 0}));
    assert(!effect_settings_set((effect_settings_t){2001, 50, 0, 0}));
    assert(!effect_settings_set((effect_settings_t){100, 101, 0, 0}));
    assert(effect_settings_get().speed_percent == 25);
    assert(effect_settings_get().brightness == 0);

    for (unsigned line = 0; line <= 4; ++line) {
        settings = (effect_settings_t){100, 6, line, 0};
        assert(effect_settings_set(settings));
        assert(effect_settings_get().define_line == line);
    }
    const char *isolated = "{\"speed_percent\":100,\"brightness\":6,\"define_line\":2}";
    assert(effect_settings_parse(isolated, strlen(isolated), &settings));
    assert(settings.define_line == 2 && effect_settings_set(settings));
    const char *legacy = "{\"speed_percent\":200,\"brightness\":6}";
    assert(effect_settings_parse(legacy, strlen(legacy), &settings) && settings.define_line == 2);
    reject_command("{\"speed_percent\":100,\"brightness\":6,\"define_line\":5}");
    reject_command("{\"speed_percent\":100,\"brightness\":6,\"define_line\":1.5}");
    reject_command("{\"speed_percent\":100,\"brightness\":6,\"define_line\":true}");
    const char *custom = "{\"speed_percent\":100,\"brightness\":100,\"define_line\":3,\"test_color\":7}";
    assert(effect_settings_parse(custom, strlen(custom), &settings));
    assert(settings.test_color == 7 && effect_settings_set(settings));
    assert(effect_settings_parse(legacy, strlen(legacy), &settings));
    assert(settings.test_color == 7 && settings.define_line == 3);
    reject_command("{\"speed_percent\":100,\"brightness\":6,\"test_color\":8}");
    assert(!effect_settings_set((effect_settings_t){100, 6, 0, 8}));
    const uint8_t expected[8][3] = {
        {0,0,0}, {255,0,0}, {0,255,0}, {0,0,255},
        {255,255,255}, {255,255,0}, {0,255,255}, {255,0,255}
    };
    const unsigned defaults[4] = {2,3,1,4};
    for (unsigned line = 0; line < 4; ++line) {
        for (unsigned color = 0; color <= 7; ++color) {
            settings = (effect_settings_t){100, 100, line + 1, color};
            uint8_t rgb[3];
            effect_rgb(settings, line, rgb);
            assert(memcmp(rgb, expected[color ? color : defaults[line]], 3) == 0);
        }
    }
    reject_command("{}");
    reject_command("[]");
    reject_command("null");
    reject_command("{\"speed_percent\":100}");
    reject_command("{\"speed_percent\":24,\"brightness\":6}");
    reject_command("{\"speed_percent\":2001,\"brightness\":6}");
    reject_command("{\"speed_percent\":100,\"brightness\":-1}");
    reject_command("{\"speed_percent\":100,\"brightness\":101}");
    reject_command("{\"speed_percent\":100.5,\"brightness\":6}");
    reject_command("{\"speed_percent\":true,\"brightness\":6}");
    reject_command("{\"speed_percent\":\"100\",\"brightness\":6}");
    reject_command("{\"speed_percent\":1e100,\"brightness\":6}");
    reject_command("{\"speed_percent\":100,\"brightness\":6} trailing");
    reject_command("{\"speed_percent\":100,\"brightness\":6,\"extra\":1}");
    reject_command("{\"speed_percent\":100,\"brightness\":6,\"brightness\":7}");
    reject_command("{\"speed_percent\":100,\"brightness\":");
    char embedded_null[] = "{\"speed_percent\":100,\"brightness\":6}\0x";
    assert(!effect_settings_parse(embedded_null, sizeof(embedded_null) - 1, &settings));
    assert(!effect_settings_parse(NULL, 0, &settings));

    for (unsigned speed = 25; speed <= 2000; ++speed) {
        for (unsigned brightness = 0; brightness <= 100; ++brightness) {
            settings = (effect_settings_t){speed, brightness, 0, 0};
            assert(effect_frame_ms(settings) >= 20 && effect_frame_ms(settings) <= 400);
            assert(effect_peak(settings) <= 255);
        }
    }
    return 0;
}
