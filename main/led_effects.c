#include "led_effects.h"

static unsigned smooth_wave(unsigned phase)
{
    unsigned value = phase % 512;
    if (value > 255) value = 511 - value;
    return value * value * (765 - 2 * value) / 65025;
}

static uint32_t noise(uint32_t seed)
{
    seed ^= seed >> 16;
    seed *= 0x7feb352dU;
    seed ^= seed >> 15;
    seed *= 0x846ca68bU;
    return seed ^ (seed >> 16);
}

static void rainbow(unsigned hue, uint8_t rgb[3])
{
    unsigned section = (hue % 1536) / 256;
    unsigned ramp = hue % 256;
    static const uint8_t channels[6][3] = {
        {255, 0, 0}, {255, 255, 0}, {0, 255, 0},
        {0, 255, 255}, {0, 0, 255}, {255, 0, 255},
    };
    unsigned next = (section + 1) % 6;
    for (unsigned channel = 0; channel < 3; ++channel) {
        rgb[channel] = (channels[section][channel] * (255 - ramp) +
                        channels[next][channel] * ramp) / 255;
    }
}

void led_effect_pixel(effect_settings_t settings, unsigned line, size_t pixel,
                      size_t count, uint64_t phase_ms, uint8_t rgb[3])
{
    uint8_t peak = effect_peak(settings);
    settings.brightness = 100;
    effect_rgb(settings, line, rgb);
    bool left = line == KITCHEN_LINE_LEFT_UP || line == KITCHEN_LINE_LEFT_DOWN;
    bool upper = line == KITCHEN_LINE_LEFT_UP || line == KITCHEN_LINE_RIGHT_UP;
    unsigned position = count > 1 ? pixel * 255 / (count - 1) : 128;
    if (left) position = 255 - position;
    /* These coordinates preserve the column order, central gap, and reversed left strips. */
    unsigned x = position + (left ? 0 : 512);
    unsigned y = upper ? 0 : 256;
    unsigned strength = 255;

    if (settings.define_line) {
        strength = settings.define_line == line + 1 ? 255 : 0;
    } else {
        switch (settings.effect) {
        case EFFECT_IDENTIFY: {
            size_t head = (phase_ms / 100) % count;
            strength = pixel <= head && head - pixel < 4 ? 255U >> (head - pixel) : 0;
            break;
        }
        case EFFECT_RAINBOW:
            rainbow((unsigned)((phase_ms / 12) % 1536) + x * 2 + y, rgb);
            break;
        case EFFECT_AURORA: {
            unsigned green = smooth_wave(x / 3 + y / 2 + (phase_ms / 17) % 512);
            unsigned violet = smooth_wave(x / 5 + 512 - (phase_ms / 29) % 512);
            rgb[0] = violet / 2;
            rgb[1] = 12 + green * 3 / 4;
            rgb[2] = 80 + violet / 3;
            break;
        }
        case EFFECT_COMETS: {
            unsigned head = ((phase_ms + (upper ? 0 : 3000)) % 6000) * 1024 / 6000;
            unsigned distance = (head + 1024 - x) % 1024;
            strength = distance < 120 ? (120 - distance) * 255 / 120 : 0;
            rainbow((unsigned)((phase_ms / 40) % 1536) + (upper ? 800 : 100), rgb);
            break;
        }
        case EFFECT_EMBERS: {
            unsigned seed = line * 4096 + pixel;
            uint32_t step = (uint32_t)(phase_ms / 300);
            unsigned blend = phase_ms % 300;
            unsigned a = noise(seed + step * 8191U) & 255;
            unsigned b = noise(seed + (step + 1) * 8191U) & 255;
            unsigned heat = 50 + (a * (300 - blend) + b * blend) * 180 / 76500;
            rgb[0] = heat;
            rgb[1] = heat * heat / 765;
            rgb[2] = 0;
            break;
        }
        case EFFECT_TWINKLE: {
            uint32_t seed = noise(line * 4096 + pixel);
            unsigned period = 4000 + seed % 3000;
            unsigned phase = (phase_ms + seed) % period;
            unsigned glow = phase < 900 ? smooth_wave(phase * 512 / 900) : 0;
            rgb[0] = glow;
            rgb[1] = glow * 210 / 255;
            rgb[2] = 2 + glow * 128 / 255;
            break;
        }
        case EFFECT_BREATHE:
            strength = smooth_wave((phase_ms % 4000) * 512 / 4000);
            break;
        case EFFECT_SOLID:
            break;
        default:
            strength = 0;
            break;
        }
    }
    for (unsigned channel = 0; channel < 3; ++channel) {
        rgb[channel] = (uint32_t)rgb[channel] * peak * strength / 65025;
    }
}
