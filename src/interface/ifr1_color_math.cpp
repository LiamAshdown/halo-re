#include "halo/interface/ifr1_color_math.hpp"
#include "halo/core/datum.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/core/x87.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/constants.hpp"
#include "halo/interface/api.hpp"

static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);

namespace halo::interface {

/**
 * Rescales a packed 0xAARRGGBB color's alpha byte by `scale`, leaving RGB untouched.
 * blam-cc: EAX -> packed_color, stack -> scale
 *
 * @address 0x497970
 */
uint32_t ColorMath::argb_scale_alpha(uint32_t packed_color, float scale)
{
    return (packed_color & halo::interface::k_rgb_mask) | (uint32_t)halo::x87::ROUND((float)(packed_color >> 0x18) * scale) << 0x18;
}

/**
 * Packs a ColorARGB (float a,r,g,b) into a 0xAARRGGBB uint32_t, rounding each channel to 0..255.
 *
 * @address 0x497900
 */
uint32_t ColorMath::pack_argb_from_real(ColorARGB *color)
{
    return (uint32_t)halo::x87::ROUND(color->blue * 255.0f) | (uint32_t)halo::x87::ROUND(color->green * 255.0f) << 8 |
           (uint32_t)halo::x87::ROUND(color->red * 255.0f) << 0x10 | (uint32_t)halo::x87::ROUND(color->alpha * 255.0f) << 0x18;
}

/**
 * Packs a 3-float {r, g, b} color (0..1 range) into a 0x00RRGGBB integer.
 *
 * @address 0x4ab5d0
 */
uint32_t ColorMath::rgb_float_to_int(const float *rgb)
{
    return ((uint32_t)(int32_t)halo::x87::ROUND(rgb[2] * 255.0f) & 0xff) |
           (((uint32_t)(int32_t)halo::x87::ROUND(rgb[1] * 255.0f) & 0xff) << 8) |
           (((uint32_t)(int32_t)halo::x87::ROUND(rgb[0] * 255.0f) & 0xff) << 0x10);
}

/**
 * Resolves the globals interface_bitmaps' table_index'th color-table TagDependency (0 = font_system, 1 =
 * font_terminal, 2 = screen_color_table, 3 = hud_color_table, 4 = editor_color_table, 5 = dialog_color_table
 * -- treating the run of leading TagDependency fields as an array), then writes out the color_index'th entry's
 * color (cycling modulo the table's actual color count) into *out. Leaves *out at opaque white (1,1,1,1) if
 * the table tag isn't assigned or has no colors.
 * blam-cc: stack -> (table_index, color_index), ECX -> out
 *
 * @address 0x494430
 */
void ColorMath::cyclic_color(int16_t table_index, int16_t color_index, ColorARGB *out)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    TagDependency *dependency;
    ColorTable *color_table;

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    dependency = (TagDependency *)((char *)interface_bitmaps + table_index * 0x10);

    out->alpha = 1.0f;
    out->red = 1.0f;
    out->green = 1.0f;
    out->blue = 1.0f;

    if (dependency->tag_id.index != halo::k_word_none || dependency->tag_id.id != halo::k_word_none) {
        color_table = (ColorTable *)halo::cache::globals().tag_instances[dependency->tag_id.index].data;
        if (color_table->colors.count != 0) {
            ColorTableColor *entry =
                (ColorTableColor *)((char *)color_table->colors.pointer +
                                     (color_index % color_table->colors.count) *
                                         sizeof(ColorTableColor));
            *out = entry->color;
        }
    }
}

}
