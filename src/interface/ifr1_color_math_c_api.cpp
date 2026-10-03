#include "halo/interface/ifr1_color_math.hpp"

/**
 * C ABI entry point; forwards to halo::interface::ColorMath::argb_scale_alpha.
 * blam-cc: EAX -> packed_color, stack -> scale
 *
 * @address 0x497970
 */
extern "C" uint32_t color_argb_scale_alpha(uint32_t packed_color, float scale)
{
    return halo::interface::ColorMath::argb_scale_alpha(packed_color, scale);
}

/**
 * C ABI entry point; forwards to halo::interface::ColorMath::pack_argb_from_real.
 *
 * @address 0x497900
 */
extern "C" uint32_t color_pack_argb_from_real(ColorARGB *color)
{
    return halo::interface::ColorMath::pack_argb_from_real(color);
}

/**
 * C ABI entry point; forwards to halo::interface::ColorMath::rgb_float_to_int.
 *
 * @address 0x4ab5d0
 */
extern "C" uint32_t color_rgb_float_to_int(const float *rgb)
{
    return halo::interface::ColorMath::rgb_float_to_int(rgb);
}

/**
 * C ABI entry point; forwards to halo::interface::ColorMath::cyclic_color.
 * blam-cc: stack -> (table_index, color_index), ECX -> out
 *
 * @address 0x494430
 */
extern "C" void globals_color_table_get_cyclic_color(int16_t table_index, int16_t color_index, ColorARGB *out)
{
    halo::interface::ColorMath::cyclic_color(table_index, color_index, out);
}
