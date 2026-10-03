/**
 * @file include/halo/bitmaps/color_codec.hpp
 * Colour conversion and interpolation helpers (packed int, RGB, HSV).
 */
#pragma once

#include "halo/bitmaps/bitmaps_types.hpp"

namespace halo::bitmaps {

/**
 * Stateless conversions between packed integer colours, floating-point RGB and HSV, and colour interpolation.
 */
struct color_codec {
    /**
     * Unpacks a 0xAARRGGBB colour into a floating-point ColorARGB with each channel scaled by 1/255.
     *
     * @address 0x43f5a0
     */
    static void argb_int_to_real(ColorARGB *out, uint32_t packed);

    /**
     * Unpacks a 0x00RRGGBB colour into a floating-point ColorRGB with each channel scaled by 1/255.
     *
     * @address 0x43f630
     */
    static void rgb_int_to_real(ColorRGB *out, uint32_t packed);

    /**
     * Expands a packed r5g6b5 word to 8 bits per channel by bit replication and writes it with alpha 0.
     *
     * @address 0x43ff80
     */
    static void unpack_565_to_rgb888(uint16_t *packed, ColorARGBInt *out);

    /**
     * Converts an RGB colour to hue, saturation and value, all in [0, 1). Returns hsv.
     *
     * @address 0x43f330
     */
    static real_hsv_color * rgb_to_hsv(ColorRGB *color, real_hsv_color *hsv);

    /**
     * Converts an HSV colour (hue scaled by 6 to pick the sextant) to RGB. Zero saturation yields grey. Returns color.
     *
     * @address 0x43f460
     */
    static ColorRGB * hsv_to_rgb(real_hsv_color *hsv, ColorRGB *color);

    /**
     * Interpolates two RGB colours by t, either channel-wise or in HSV space, optionally taking the longer hue path.
     * Returns dest.
     *
     * @address 0x43f6a0
     */
    static ColorRGB * interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, color_interpolation_flags flags, float t);

    /**
     * Interpolates two ARGB colours and then blends the result with an optional tint, weighted by the interpolated alpha.
     * Returns dest.
     *
     * @address 0x43f7d0
     */
    static ColorRGB * interpolate_argb_with_tint(color_interpolation_flags flags, ColorARGB *color1, ColorRGB *dest, ColorRGB *tint, ColorARGB *color0, float t);

};

}  // namespace halo::bitmaps
