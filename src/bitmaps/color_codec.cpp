/**
 * @file src/bitmaps/color_codec.cpp
 * Colour conversion and interpolation helpers (packed int, RGB, HSV).
 * The original author notes and decompiles are in docs/original/bitmaps/.
 */

#include "halo/bitmaps/bitmaps.hpp"

extern "C" {
extern float fabsf(float x);
}

namespace halo::bitmaps {

void color_codec::argb_int_to_real(ColorARGB *out, uint32_t packed)
{
    out->alpha = (float)((packed >> 24) & 0xff) * 0.003921569f;
    out->red   = (float)((packed >> 16) & 0xff) * 0.003921569f;
    out->green = (float)((packed >> 8) & 0xff) * 0.003921569f;
    out->blue  = (float)(packed & 0xff) * 0.003921569f;
}

void color_codec::rgb_int_to_real(ColorRGB *out, uint32_t packed)
{
    out->red   = (float)((packed >> 16) & 0xff) * 0.003921569f;
    out->green = (float)((packed >> 8) & 0xff) * 0.003921569f;
    out->blue  = (float)(packed & 0xff) * 0.003921569f;
}

void color_codec::unpack_565_to_rgb888(uint16_t *packed, ColorARGBInt *out)
{
    uint16_t value;
    uint8_t r5, g6, b5;

    value = *packed;
    r5 = (uint8_t)((value >> 11) & 0x1f);
    g6 = (uint8_t)((value >> 5) & 0x3f);
    b5 = (uint8_t)(value & 0x1f);

    out->blue = (uint8_t)((b5 << 3) | (b5 >> 2));
    out->green = (uint8_t)((g6 << 2) | (g6 >> 4));
    out->red = (uint8_t)((r5 << 3) | (r5 >> 2));
    out->alpha = 0;
}

real_hsv_color * color_codec::rgb_to_hsv(ColorRGB *color, real_hsv_color *hsv)
{
    float max, min, delta;
    float hue;

    max = color->red;
    if (max < color->green) max = color->green;
    if (max < color->blue) max = color->blue;

    min = color->red;
    if (min > color->green) min = color->green;
    if (min > color->blue) min = color->blue;

    delta = max - min;
    hsv->value = max;
    hsv->saturation = (max == 0.0f) ? 0.0f : delta / max;

    if (hsv->saturation == 0.0f) {
        hsv->hue = 0.0f;
        return hsv;
    }

    if (color->red == max) {
        hue = (color->green - color->blue) / delta;
    } else if (color->green == max) {
        hue = (color->blue - color->red) / delta + 2.0f;
    } else {
        hue = (color->red - color->green) / delta + 4.0f;
    }

    hue = hue * 0.16666667f;
    if (hue < 0.0f) {
        hue = hue + 1.0f;
    }
    hsv->hue = hue;
    return hsv;
}

ColorRGB * color_codec::hsv_to_rgb(real_hsv_color *hsv, ColorRGB *color)
{
    if (hsv->saturation != 0.0f) {
        float hue6 = hsv->hue * 6.0f;
        int32_t sector = (int32_t)hue6;
        float frac;
        float p, q, t;

        if ((float)sector > hue6) {
            sector -= 1;
        }
        frac = hue6 - (float)sector;

        p = (1.0f - hsv->saturation) * hsv->value;
        q = (1.0f - frac * hsv->saturation) * hsv->value;
        t = (1.0f - (1.0f - frac) * hsv->saturation) * hsv->value;

        switch (sector) {
        case 0: color->red = hsv->value; color->green = t;         color->blue = p;         break;
        case 1: color->red = q;          color->green = hsv->value; color->blue = p;         break;
        case 2: color->red = p;          color->green = hsv->value; color->blue = t;         break;
        case 3: color->red = p;          color->green = q;          color->blue = hsv->value; break;
        case 4: color->red = t;          color->green = p;          color->blue = hsv->value; break;
        case 5: color->red = hsv->value; color->green = p;          color->blue = q;         break;
        default: break;
        }
    } else {
        color->red = hsv->value;
        color->green = hsv->value;
        color->blue = hsv->value;
    }
    return color;
}

ColorRGB * color_codec::interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, color_interpolation_flags flags, float t)
{
    float one_minus_t = 1.0f - t;

    if ((flags & _color_interpolation_hsv_bit) == 0) {
        dest->red   = t * color1->red   + one_minus_t * color0->red;
        dest->green = t * color1->green + one_minus_t * color0->green;
        dest->blue  = t * color1->blue  + one_minus_t * color0->blue;
        return dest;
    }

    {
        real_hsv_color hsv0, hsv1, hsv;
        uint8_t hue_far_apart;
        uint8_t take_long_path;

        color_codec::rgb_to_hsv(color0, &hsv0);
        color_codec::rgb_to_hsv(color1, &hsv1);

        hue_far_apart = fabsf(hsv0.hue - hsv1.hue) > 0.5f;
        take_long_path = (uint8_t)((flags >> 1) & 1);

        if (hue_far_apart != take_long_path) {
            if (hsv0.hue < hsv1.hue) {
                hsv0.hue = hsv0.hue + 1.0f;
            } else {
                hsv1.hue = hsv1.hue + 1.0f;
            }
        }

        hsv.hue = one_minus_t * hsv0.hue + t * hsv1.hue;
        if (hsv.hue > 1.0f) {
            hsv.hue = hsv.hue - 1.0f;
        }
        hsv.saturation = one_minus_t * hsv0.saturation + t * hsv1.saturation;
        hsv.value      = one_minus_t * hsv0.value      + t * hsv1.value;

        color_codec::hsv_to_rgb(&hsv, dest);
    }
    return dest;
}

ColorRGB * color_codec::interpolate_argb_with_tint(color_interpolation_flags flags, ColorARGB *color1, ColorRGB *dest, ColorRGB *tint, ColorARGB *color0, float t)
{
    color_codec::interpolate((ColorRGB *)&color1->red, (ColorRGB *)&color0->red, dest, flags, t);

    if (tint != 0) {
        if (color0->alpha <= 0.0001f && color1->alpha <= 0.0001f) {
            dest->red   = dest->red * tint->red;
            dest->green = tint->green * dest->green;
            dest->blue  = tint->blue * dest->blue;
        } else {
            float weight = t * color1->alpha + (1.0f - t) * color0->alpha;
            float inverse_weight = 1.0f - weight;
            dest->red   = weight * dest->red   + inverse_weight * tint->red;
            dest->green = weight * dest->green + inverse_weight * tint->green;
            dest->blue  = weight * dest->blue  + inverse_weight * tint->blue;
        }
    }
    return dest;
}

}  // namespace halo::bitmaps
