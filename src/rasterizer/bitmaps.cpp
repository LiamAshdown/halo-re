/**
 * @file src/rasterizer/bitmaps.cpp
 * Bitmap upload, texture binding and sampler state helpers.
 * The original author notes and decompiles are in docs/original/rasterizer/.
 */

#include "halo/render/d3d9.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/state.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/bitmaps/bitmaps.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"




namespace halo::rasterizer {

/**
 * 0x4cb740; ECX -> value Computes the number of mipmap levels to generate for a bitmap, honouring its
 * requested mipmap_count when the bitmap's dimensions can naturally support at least that many levels. the
 * result is a 16-bit level count: on the mipmap_count path the original loads only AX, leaving the upper half
 * of EAX from earlier code; its caller reads AX
 *
 * Registers: unaff_EBX -> bitmap
 *
 * @address 0x5145a0
 */
int16_t bitmap_compute_mipmap_count(BitmapData *bitmap)
{
    uint16_t flags = bitmap->flags;
    uint32_t result = 0;

    if ((flags & 1) == 0 || (flags & 0x10) != 0) {
        return result;
    }

    if ((flags & 2) == 0) {
        int16_t height = (int16_t)bitmap->height;
        int16_t depth = (int16_t)bitmap->depth;
        int16_t max_hd = (height <= depth) ? depth : height;
        int16_t width = (int16_t)bitmap->width;
        int32_t max_dim;
        int16_t levels;

        if (width <= max_hd) {
            max_dim = (height <= depth) ? depth : height;
        } else {
            max_dim = width;
        }

        levels = 0;
        if (max_dim != 0) {
            uint32_t v = (uint32_t)max_dim;
            while (v != 1) {
                v = v >> 1;
                levels = levels + 1;
            }
        }

        result = bitmap->mipmap_count;
        if (levels < (int16_t)bitmap->mipmap_count) {
            int16_t hd = (height <= depth) ? depth : height;
            if (hd < width) {
                return (uint32_t)halo::math::uint32_log2_floor((uint32_t)max_dim);
            }
            return (uint32_t)halo::math::uint32_log2_floor((uint32_t)max_dim);
        }
    } else {
        int16_t depth = (int16_t)bitmap->depth;
        int32_t width_blocks = ((int32_t)(int16_t)bitmap->width +
                                 (((int32_t)(int16_t)bitmap->width >> 0x1f) & 3)) >> 2;
        int32_t max1 = (width_blocks <= depth) ? depth : width_blocks;
        int32_t height_rounded = (int32_t)(int16_t)bitmap->height +
                                  (((int32_t)(int16_t)bitmap->height >> 0x1f) & 3);
        int32_t height_blocks = height_rounded >> 2;
        int32_t max2;
        int16_t levels;

        if (height_blocks <= max1) {
            max2 = (width_blocks <= depth) ? depth : width_blocks;
        } else {
            max2 = height_blocks;
        }

        levels = 0;
        if (max2 != 0) {
            uint32_t v = (uint32_t)max2;
            while (v != 1) {
                v = v >> 1;
                levels = levels + 1;
            }
        }

        result = bitmap->mipmap_count;
        if (levels < (int16_t)bitmap->mipmap_count) {
            int32_t wb = width_blocks;
            if (wb <= depth) {
                wb = depth;
            }
            if (wb < height_blocks) {
                return (uint32_t)halo::math::uint32_log2_floor((uint32_t)max2);
            }
            return (uint32_t)halo::math::uint32_log2_floor((uint32_t)max2);
        }
    }
    return result;
}

/**
 * Computes the total byte size of a bitmap's pixel data across every mip level (see the file header for the
 * compressed-format and cube-map adjustments), rounded up to a multiple of 0x80 bytes.
 *
 * Registers: in_EAX -> bitmap
 *
 * @address 0x5146c0
 */
int32_t bitmap_compute_texture_data_size(BitmapData *bitmap)
{
    int32_t total = 0;
    int32_t level_bytes = 0;
    int16_t mip_count;
    int16_t level;
    uint8_t shift;
    int8_t bits_per_pixel;
    uint16_t flags;

    mip_count = (int16_t)bitmap_compute_mipmap_count(bitmap);

    if (mip_count >= 0) {
        bits_per_pixel = bitmap_format_bits_per_pixel[bitmap->format];
        flags = bitmap->flags;
        level = 0;
        shift = 0;
        do {
            level_bytes = (int32_t)halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_level_pixel_count(level);
            level_bytes = level_bytes * bits_per_pixel;
            level_bytes = (level_bytes + ((level_bytes >> 0x1f) & 7)) >> 3;

            if ((flags & 0x10) != 0) {
                int16_t width_at_level;
                int16_t height_at_level;
                int32_t height_bytes;

                width_at_level = ((int16_t)bitmap->width >> shift) < 2 ? 1 : (int16_t)((int16_t)bitmap->width >> shift);
                if ((bitmap->flags & 2) != 0) {
                    width_at_level = width_at_level + ((uint8_t)(-(int8_t)width_at_level) & 3);
                }
                height_bytes = (int32_t)width_at_level * (int32_t)bits_per_pixel;

                height_at_level = ((int16_t)bitmap->height >> shift) < 2 ? 1 : (int16_t)((int16_t)bitmap->height >> shift);
                if ((bitmap->flags & 2) != 0) {
                    height_at_level = height_at_level + ((uint8_t)(-(int8_t)height_at_level) & 3);
                }
                level_bytes = level_bytes + (int32_t)height_at_level *
                              (-(((height_bytes + ((height_bytes >> 0x1f) & 7)) >> 3)) & 0x3f);
            }

            if (bitmap->type == 2) {
                level_bytes = level_bytes / 6;
            }
            total = total + level_bytes;
            level = level + 1;
            shift = shift + 1;
        } while (level <= mip_count);
    }

    total = total + (-total & 0x7f);
    if (bitmap->type != 2) {
        return total;
    }
    return total * 6;
}

static BitmapData *rasterizer_default_bitmap(int16_t bitmap_type, int16_t default_index)
{
    uint32_t tag = *(uint32_t *)((uint8_t *)rasterizer_globals_data + bitmap_type * 0x10 + 0xb8);
    Bitmap *bitmap;

    if (tag == halo::k_dword_none) {
        return 0;
    }
    bitmap = (Bitmap *)halo::cache::globals().tag_instances[tag & halo::k_slot_mask].data;
    if (bitmap == 0 || default_index < 0 || default_index >= (int32_t)bitmap->bitmap_data.count) {
        return 0;
    }
    return (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + default_index * 0x30);
}

static BitmapData *rasterizer_tag_bitmap(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t default_index, int16_t frame,
                                         uint8_t *resolved)
{
    Bitmap *bitmap;
    int32_t count;

    *resolved = 0;
    if ((halo::rasterizer::fields::bump_mapping_enabled == 0 && default_index == 3) || bitmap_tag_id == halo::k_dword_none) {
        return 0;
    }
    bitmap = (Bitmap *)halo::cache::globals().tag_instances[bitmap_tag_id & halo::k_slot_mask].data;
    count = (int32_t)bitmap->bitmap_data.count;
    if (count <= 0) {
        return 0;
    }
    *resolved = 1;
    return halo::bitmaps::bitmap_group_get_bitmap_data(bitmap_tag_id, (int16_t)((int32_t)frame % count));
}

/**
 * Direct3D 9 back end function chimera__rasterizer_set_texture. The original author notes are in
 * docs/original/rasterizer/chimera__rasterizer_set_texture.c.txt.
 *
 * Registers: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)
 *
 * @address 0x518960
 */
int16_t * chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type, int16_t default_index, int16_t frame)
{
    uint8_t resolved;
    BitmapData *data = rasterizer_tag_bitmap(bitmap_tag_id, bitmap_type, default_index, frame, &resolved);

    if (!resolved || *(int16_t *)&((struct BitmapData *)data)->type != bitmap_type) {
        data = rasterizer_default_bitmap(bitmap_type, default_index);
        if (data == 0) {
            return 0;
        }
    }
    rasterizer_bind_texture_d3d9(stage, data);
    rasterizer_bound_bitmap_size_a[0] = (int16_t)data->width;
    rasterizer_bound_bitmap_size_a[1] = (int16_t)data->height;
    return rasterizer_bound_bitmap_size_a;
}

static BitmapData *bitmap_group_frame(uint32_t bitmap_tag_id, int16_t frame)
{
    Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bitmap_tag_id & halo::k_slot_mask].data;
    int32_t count = (int32_t)bitmap->bitmap_data.count;
    int16_t index;

    if (count <= 0 || bitmap == 0) {
        return 0;
    }
    index = (int16_t)((int32_t)frame % count);
    if (index < 0 || index >= count) {
        return 0;
    }
    return (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + index * 0x30);
}

/**
 * Direct3D 9 back end function chimera__rasterizer_set_texture_direct_d3d9. The original author notes are in
 * docs/original/rasterizer/chimera__rasterizer_set_texture_direct_d3d9.c.txt.
 *
 * Registers: EAX -> bitmap_tag_id, stack -> (stage, frame)
 *
 * @address 0x518770
 */
uint8_t chimera__rasterizer_set_texture_direct_d3d9(uint32_t bitmap_tag_id, int16_t stage, int16_t frame)
{
    BitmapData *data;

    if (bitmap_tag_id == halo::k_dword_none) {
        return 0;
    }
    data = bitmap_group_frame(bitmap_tag_id, frame);
    if (data == 0) {
        return 0;
    }
    rasterizer_bind_texture_d3d9(stage, data);
    return 1;
}

/**
 * Direct3D 9 back end function chimera__rasterizer_set_texture_direct_d3dx. The original author notes are in
 * docs/original/rasterizer/chimera__rasterizer_set_texture_direct_d3dx.c.txt.
 *
 * Registers: EAX -> bitmap_tag_id, EDI -> effect_slot, stack -> (stage, frame)
 *
 * @address 0x518700
 */
uint8_t chimera__rasterizer_set_texture_direct_d3dx(uint32_t bitmap_tag_id, int16_t stage, int16_t frame, rasterizer_effect_slot *effect_slot)
{
    BitmapData *data;

    if (bitmap_tag_id == halo::k_dword_none) {
        return 0;
    }
    data = bitmap_group_frame(bitmap_tag_id, frame);
    if (data == 0) {
        return 0;
    }
    rasterizer_bind_texture_d3dx(stage, data, effect_slot);
    return 1;
}

/**
 * harness/x87_shims.c: FISTP in the current (round-to-nearest-even) mode VERIFIED against disassembly
 * 0x5132b0..0x5132ca (2026-09-30): the original is fmul 255.0, fstp dword, fld, FISTP (round to nearest, ties
 * to even, NOT `+ 0.5` then truncate: they differ on exact halves and on negatives) and returns only AL. The
 * multiply can overflow past 255 for an out-of-range input; the cast to uint8_t keeps the low byte exactly as
 * the original's `mov al, [esp]` does.
 *
 * @address 0x5132b0
 */
uint8_t color_channel_real_to_byte(float channel)
{
    return (uint8_t)halo::x87::fistp_round(channel * 255.0f);
}


/**
 * Direct3D 9 back end function rasterizer_bind_texture_d3d9. The original author notes are in
 * docs/original/rasterizer/rasterizer_bind_texture_d3d9.c.txt.
 *
 * Registers: ESI -> bitmap, stack -> stage
 *
 * @address 0x518680
 */
uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap)
{

    if (bitmap == 0) {
        return 0;
    }
    halo::cache::texture_cache_get(bitmap, 1, 1);
    if (render_device().set_texture((uint32_t)(int32_t)stage, *&((struct BitmapData *)bitmap)->hardware_texture) < 0) {
        return 0;
    }
    return 1;
}

namespace rasterizer_bind_texture_d3dx_impl {


/**
 * Direct3D 9 back end function rasterizer_bind_texture_d3dx. The original author notes are in
 * docs/original/rasterizer/rasterizer_bind_texture_d3dx.c.txt.
 *
 * Registers: ESI -> bitmap, EDI -> effect_slot, stack -> stage
 *
 * @address 0x5186c0
 */
uint8_t rasterizer_bind_texture_d3dx(int16_t stage, BitmapData *bitmap, rasterizer_effect_slot *effect_slot)
{
    void *effect;

    if (bitmap == 0) {
        return 0;
    }
    halo::cache::texture_cache_get(bitmap, 1, 1);
    effect = effect_slot->effect;
    render_device().effect_set_texture(effect, effect_slot->texture_handles[stage], *&((struct BitmapData *)bitmap)->hardware_texture);
    return 1;
}

}  // namespace rasterizer_bind_texture_d3dx_impl

/**
 * Computes how many top mip levels the bitmap skips for the texture quality setting (renderer_texture_quality at
 * 0x0068944e: 0 high, 1 medium, 2 low), clamped to [0,2]. Returns how many mip levels were skipped (0 if the
 * quality setting, bitmap flags or mip count don't allow skipping), halving *out_width/*out_height once per
 * level skipped.
 *
 * @address 0x523f10
 */
int32_t rasterizer_bitmap_compute_mipmap_skip_count(BitmapData *bitmap, int16_t *out_width, int16_t *out_height)
{
    int32_t max_skip;
    int32_t mipmap_count;
    int32_t remaining;

    if (renderer_texture_quality < 0) {
        max_skip = 0;
    } else if (renderer_texture_quality < 3) {
        max_skip = renderer_texture_quality;
    } else {
        max_skip = 2;
    }

    mipmap_count = bitmap->mipmap_count;
    *out_width = bitmap->width;
    *out_height = bitmap->height;

    if (max_skip != 0 && 0x3f < *out_width && 0x3f < *out_height && 1 < mipmap_count) {
        remaining = max_skip;
        for (; 0 < max_skip; max_skip--) {
            if (max_skip < mipmap_count) {
                *out_width = *out_width / 2;
                *out_height = *out_height / 2;
                mipmap_count = mipmap_count - 1;
            } else {
                remaining = remaining - 1;
            }
        }
        return remaining;
    }
    return 0;
}

namespace rasterizer_bitmap_create_hardware_texture_impl {


typedef int32_t (__stdcall *d3d_create_volume_texture_fn)(void *self, uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, int32_t format, uint32_t pool, void *out_texture, void *shared_handle);

typedef int32_t (__stdcall *d3d_create_cube_texture_fn)(void *self, uint32_t edge_length, uint32_t levels, uint32_t usage, int32_t format, uint32_t pool, void *out_texture, void *shared_handle);

/**
 * Creates the hardware texture/volume texture/cube texture object for `bitmap` (matching its type field) and
 * stores it in bitmap->hardware_texture. Returns 1 on success (including "nothing to do" cases: no device, no
 * format mapping, or an unsupported capability), 0 on a real failure.
 *
 * Registers: ESI = bitmap
 *
 * @address 0x523fa0
 */
uint8_t rasterizer_bitmap_create_hardware_texture(BitmapData *bitmap)
{
    uint8_t ok;
    int32_t format;
    int32_t hresult;
    int16_t width, height;
    int32_t mip_skip;
    int32_t levels;

    ok = 1;
    if (rasterizer_device == 0) {
        bitmap->hardware_texture = 0;
        return 1;
    }

    format = rasterizer_bitmap_format_to_d3dformat[bitmap->format];
    if (format == -1) {
        bitmap->hardware_texture = 0;
        return 1;
    }

    if (bitmap->type == 0) {
        mip_skip = rasterizer_bitmap_compute_mipmap_skip_count(bitmap, &width, &height);
        hresult = render_device().create_texture(width, height, (uint32_t)((bitmap->mipmap_count - mip_skip) + 1), 0, format, 1, &bitmap->hardware_texture, 0);
        if (hresult < 0) {
            ok = 0;
        }
    } else if (bitmap->type == 1) {
        if (!halo::d3d9::has_texture_cap(rasterizer_caps.texture_caps, halo::d3d9::texture_cap::volume_map)) {
            bitmap->hardware_texture = 0;
        } else {
            levels = ((int8_t)(rasterizer_caps.texture_caps >> 8) < 0) ? bitmap->mipmap_count + 1 : 1;
            d3d_create_volume_texture_fn create_volume_texture =
                (d3d_create_volume_texture_fn)(*(void ***)rasterizer_device)[0x18];
            hresult = create_volume_texture(rasterizer_device, bitmap->width, bitmap->height,
                bitmap->depth, (uint32_t)levels, 0, format, 1, &bitmap->hardware_texture, 0);
            if (hresult < 0) {
                ok = 0;
            }
        }
    } else if (bitmap->type == 2) {
        if (!halo::d3d9::has_texture_cap(rasterizer_caps.texture_caps, halo::d3d9::texture_cap::cube_map)) {
            bitmap->hardware_texture = 0;
        } else {
            levels = !halo::d3d9::has_texture_cap(rasterizer_caps.texture_caps, halo::d3d9::texture_cap::mip_cube_map) ? 1 : bitmap->mipmap_count + 1;
            d3d_create_cube_texture_fn create_cube_texture =
                (d3d_create_cube_texture_fn)(*(void ***)rasterizer_device)[0x19];
            hresult = create_cube_texture(rasterizer_device, bitmap->width, (uint32_t)levels, 0,
                format, 1, &bitmap->hardware_texture, 0);
            if (hresult < 0) {
                ok = 0;
            }
        }
    }

    if (bitmap->hardware_texture == 0) {
        bitmap->hardware_texture = 0;
        return 0;
    }
    if (ok == 0) {
        bitmap->hardware_texture = 0;
    }
    return ok;
}

}  // namespace rasterizer_bitmap_create_hardware_texture_impl

typedef struct locked_rect {
    int32_t pitch;
    uint8_t *bits;
} locked_rect;

static int32_t sample_texel_coordinate(int32_t size, float uv)
{
    float scaled = (float)size * uv - 0.5f;
    int32_t texel = (int32_t)halo::libm::lrint((double)scaled);

    if ((size & (size - 1)) == 0) {
        return texel & (size - 1);
    }
    return (texel % size + size) % size;
}

static int16_t level_dimension(uint16_t base, int16_t level, int16_t skip, uint8_t compressed)
{
    int16_t size = ((int16_t)base >> level) > 1 ? (int16_t)((int16_t)base >> level) : 1;

    if (compressed) {
        size = (int16_t)(size + ((uint8_t)(-(uint8_t)size) & 3));
    }
    return (int16_t)(size >> skip);
}

/**
 * Direct3D 9 back end function rasterizer_bitmap_sample_texel. The original author notes are in
 * docs/original/rasterizer/rasterizer_bitmap_sample_texel.c.txt.
 *
 * @address 0x524590
 */
int32_t rasterizer_bitmap_sample_texel(BitmapData *bitmap, float *uv, float mip_bias)
{
    uint8_t *data = (uint8_t *)bitmap;
    void *texture = *(void **)&((struct BitmapData *)data)->hardware_texture;
    int16_t skipped_width;
    int16_t skipped_height;
    int16_t skip;
    int16_t level;
    int16_t remaining;
    uint8_t compressed;
    int32_t width;
    int32_t height;
    int32_t x;
    int32_t y;
    int32_t texel;
    locked_rect locked;
    void **vtable;

    if (texture == 0) {
        return -1;
    }
    skip = (int16_t)rasterizer_bitmap_compute_mipmap_skip_count(bitmap, &skipped_width, &skipped_height);
    remaining = (int16_t)(*(int16_t *)&((struct BitmapData *)data)->mipmap_count - skip);
    level = 0;
    if (mip_bias < 1.0f && remaining > 0 && skip == 0) {
        float mip = (1.0f - mip_bias) * (float)remaining;

        level = (int16_t)halo::libm::lrint((double)mip);
    }
    compressed = (uint8_t)((*(uint16_t *)&((struct BitmapData *)data)->flags & 2) != 0);
    width = level_dimension(((struct BitmapData *)data)->width, level, skip, compressed);
    height = level_dimension(((struct BitmapData *)data)->height, level, skip, compressed);
    x = sample_texel_coordinate(width, uv[0]);
    y = sample_texel_coordinate(height, uv[1]);

    vtable = *(void ***)texture;
    if (((int32_t (__stdcall *)(void *, uint32_t, locked_rect *, void *, uint32_t))vtable[0x4c / 4])(
            texture, (uint32_t)level, &locked, 0, 0x810) < 0) {
        return -1;
    }
    if (locked.bits == 0) {
        return width;
    }

    texel = width;
    if (compressed) {
        int16_t format = *(int16_t *)&((struct BitmapData *)data)->format;
        int32_t block_bytes = ((int32_t)bitmap_format_bits_per_pixel[format] * 16) / 8;
        int32_t block_row = (int16_t)(y / 4);
        int32_t block_index = (block_row * width) / 4 + (int16_t)(x / 4);
        uint8_t *block = locked.bits + block_index * (int16_t)block_bytes;

        x &= 3;
        y &= 3;
        switch (format) {
        case 0xe: halo::bitmaps::dxt_decoder::decode_dxt1_texel(reinterpret_cast<ColorARGBInt *>(&texel), reinterpret_cast<dxt_color_block *>(block), x, y); break;
        case 0xf: halo::bitmaps::dxt_decoder::decode_dxt3_texel(x, y, reinterpret_cast<ColorARGBInt *>(&texel), reinterpret_cast<dxt3_block *>(block)); break;
        case 0x10: halo::bitmaps::dxt_decoder::decode_dxt5_texel(reinterpret_cast<dxt5_block *>(block), reinterpret_cast<ColorARGBInt *>(&texel), x, y); break;
        default: break;
        }
    } else {
        uint8_t *row = locked.bits + y * locked.pitch;
        uint32_t v;
        uint32_t a;
        uint32_t b;
        uint32_t c;
        uint32_t d;

        switch (*(int16_t *)&((struct BitmapData *)data)->format) {
        case 6:
            v = *(uint16_t *)(row + x * 2);
            a = ((v & 0xfffff800) | 0xffff0000) << 3;
            a |= v & 0x7e0;
            b = (v >> 1) & 0xe;
            a <<= 2;
            a |= v & 0xffffe01f;
            b |= v & 0x600;
            a <<= 3;
            b >>= 1;
            texel = (int32_t)(a | b);
            break;
        case 8:
            v = *(uint16_t *)(row + x * 2);
            a = (v & 0x7c00) << 3;
            a |= v & 0x3e0;
            a <<= 2;
            a |= v & 0x7000;
            a <<= 1;
            a |= v & 0x1f;
            a <<= 2;
            a |= v & 0x380;
            a <<= 1;
            a |= (v >> 2) & 7;
            a |= (uint32_t)(-(int32_t)(v >> 15)) << 24;
            texel = (int32_t)a;
            break;
        case 9:
            v = *(uint16_t *)(row + x * 2);
            a = v >> 8;
            b = a & 0xf;
            a = ((a & 0xfffffff0) << 12) | v;
            a &= 0xfffff000;
            c = ((b << 4) | b) << 4;
            a |= c;
            d = (v >> 4) & 0xf;
            a |= d;
            a = (a << 4) | d;
            a <<= 4;
            a |= v & 0xf;
            a = (a << 4) | (v & 0xf);
            texel = (int32_t)a;
            break;
        case 0xa:
            texel = *(int32_t *)(row + x * 4);
            break;
        case 0xb:
            texel = *(row + x * 4);
            break;
        default:
            texel = -1;
            break;
        }
    }
    ((int32_t (__stdcall *)(void *, uint32_t))vtable[0x50 / 4])(texture, (uint32_t)level);
    return texel;
}

namespace rasterizer_bitmap_upload_2d_mipmaps_impl {



/**
 * D3DLOCKED_RECT
 *
 * Registers: param_1 = bitmap
 *
 * @address 0x524100
 */
void rasterizer_bitmap_upload_2d_mipmaps(BitmapData *bitmap)
{
    uint8_t ok;
    int32_t mip_skip;
    int16_t out_width, out_height;
    int16_t level;
    int32_t source_mip;
    d3d_locked_rect locked;
    int32_t hresult;
    uint8_t *dest;
    uint8_t *source;
    uint32_t row, rows, row_size;
    uint32_t level_size;

    ok = 1;
    mip_skip = rasterizer_bitmap_compute_mipmap_skip_count(bitmap, &out_width, &out_height);

    if (rasterizer_device == 0 || *(uint32_t *)&((struct BitmapData *)bitmap)->pixel_base == 0 ||
        bitmap->hardware_texture == 0) {
        return;
    }

    source_mip = mip_skip;
    for (level = 0; ok; level++, source_mip++) {
        if (bitmap->mipmap_count - mip_skip < level) {
            return;
        }

        hresult = render_device().texture_lock_rect(bitmap->hardware_texture, (uint32_t)level, &locked, 0, 0);

        if (hresult < 0 || locked.bits == 0) {
            ok = 0;
            break;
        }

        source = (uint8_t *)halo::bitmaps::bitmap_data_view(bitmap).get_pixel_address(source_mip);

        if ((bitmap->flags & 2) == 0) {
            rows = halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_dimension(source_mip);
            row_size = halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_row_byte_size(source_mip);
            dest = (uint8_t *)locked.bits;
            for (row = 0; row < rows; row++) {
                memcpy(dest, source, row_size);
                source = source + row_size;
                dest = dest + locked.pitch;
            }
        } else {
            level_size = halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_level_byte_size(source_mip);
            memcpy((void *)locked.bits, source, level_size);
        }

        hresult = render_device().texture_unlock_rect(bitmap->hardware_texture, (uint32_t)level);
        if (hresult < 0) {
            ok = 0;
        }
    }
}

}  // namespace rasterizer_bitmap_upload_2d_mipmaps_impl

namespace rasterizer_bitmap_upload_cubemap_mipmaps_impl {

typedef struct d3d_locked_box {
    int32_t row_pitch;
    int32_t slice_pitch;
    void *bits;
} d3d_locked_box;



/**
 * REWRITTEN (objdump 0x524270..0x5243b7, 2026-09-25): this uploads a VOLUME (3D) texture, one depth slice at a
 * time. The draft called LockBox/UnlockBox with an extra argument (these are __stdcall: the stack drifted),
 * copied a whole level per slice instead of level/depth, and stepped the destination by the row pitch instead
 * of the slice pitch.
 *
 * Registers: EBX -> bitmap
 *
 * @address 0x524270
 */
void rasterizer_bitmap_upload_cubemap_mipmaps(BitmapData *bitmap)
{
    uint8_t ok = 1;
    int16_t max_level;
    int16_t level;
    int16_t depth, slice;
    d3d_locked_box locked;
    uint8_t *source;
    uint8_t *dest;
    int32_t level_bytes, slice_bytes;

    if (rasterizer_device == 0 || *(uint32_t *)&((struct BitmapData *)bitmap)->pixel_base == 0 || bitmap->hardware_texture == 0) {
        return;
    }

    max_level = ((int8_t)(rasterizer_caps.texture_caps >> 8) < 0) ? bitmap->mipmap_count : 0;

    for (level = 0; ok && level <= max_level; level++) {
        if (render_device().volume_texture_lock_box(bitmap->hardware_texture, (uint32_t)level, &locked, 0, 0) < 0 ||
            locked.bits == 0) {
            ok = 0;
            continue;
        }
        source = (uint8_t *)halo::bitmaps::bitmap_data_view(bitmap).get_pixel_address(level);
        depth = halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_depth(level);
        dest = (uint8_t *)locked.bits;
        for (slice = 0; slice < depth; slice++) {

            level_bytes = (int32_t)halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_level_pixel_count(level) *
                          bitmap_format_bits_per_pixel[bitmap->format];
            level_bytes = level_bytes / 8;
            slice_bytes = level_bytes / depth;
            memcpy(dest, source, slice_bytes);
            source += slice_bytes;
            dest += locked.slice_pitch;
        }
        if (render_device().volume_texture_unlock_box(bitmap->hardware_texture, (uint32_t)level) < 0) {
            ok = 0;
        }
    }
}

}  // namespace rasterizer_bitmap_upload_cubemap_mipmaps_impl

namespace rasterizer_bitmap_upload_cubemap_mipmaps_by_face_impl {



/**
 * REWRITTEN (objdump 0x5243c0..0x52458a, 2026-09-25). For each mip level (every level only if the card reports
 * D3DPTEXTURECAPS_MIPCUBEMAP, TextureCaps bit 16) and each of the six faces: LockRect(D3D face, level), copy
 * the face (as one block of level bytes / 6 for compressed formats, else row by row at the locked pitch),
 * UnlockRect.
 *
 * Registers: stack -> bitmap
 *
 * @address 0x5243c0
 */
void rasterizer_bitmap_upload_cubemap_mipmaps_by_face(BitmapData *bitmap)
{
    uint8_t ok = 1;
    int16_t max_level, level, face;
    d3d_locked_rect locked;
    uint8_t *source;
    uint8_t *dest;
    int16_t rows, row;
    uint32_t row_size;
    int32_t bytes;

    if (rasterizer_device == 0 || *(uint32_t *)&((struct BitmapData *)bitmap)->pixel_base == 0 || bitmap->hardware_texture == 0) {
        return;
    }
    max_level = halo::d3d9::has_texture_cap(rasterizer_caps.texture_caps, halo::d3d9::texture_cap::mip_cube_map) ? bitmap->mipmap_count : 0;

    for (level = 0; ok && level <= max_level; level++) {
        for (face = 0; ok && face < 6; face++) {
            if (render_device().cube_texture_lock_rect(bitmap->hardware_texture, (uint32_t)rasterizer_cube_face_to_d3d_face[face], (uint32_t)level, &locked, 0, 0) < 0 ||
                locked.bits == 0) {
                ok = 0;
                continue;
            }
            dest = (uint8_t *)locked.bits;
            source = (uint8_t *)halo::bitmaps::bitmap_data_view(bitmap).get_cube_map_pixel_address(level, 0, 0, face);
            if ((bitmap->flags & 2) != 0) {

                bytes = (int32_t)halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_level_pixel_count(level) *
                        bitmap_format_bits_per_pixel[bitmap->format];
                bytes = bytes / 8;
                memcpy(dest, source, bytes / 6);
            } else {
                rows = (int16_t)halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_dimension(level);
                row_size = halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_row_byte_size(level);
                for (row = 0; row < rows; row++) {
                    memcpy(dest, source, row_size);
                    source += row_size;
                    dest += locked.pitch;
                }
            }
            if (render_device().cube_texture_unlock_rect(bitmap->hardware_texture, (uint32_t)rasterizer_cube_face_to_d3d_face[face], (uint32_t)level) < 0) {
                ok = 0;
            }
        }
    }
}

}  // namespace rasterizer_bitmap_upload_cubemap_mipmaps_by_face_impl

namespace rasterizer_force_bilinear_filtering_impl {



/**
 * Direct3D 9 back end function rasterizer_force_bilinear_filtering. The original author notes are in
 * docs/original/rasterizer/rasterizer_force_bilinear_filtering.c.txt.
 *
 * @address 0x51e9f0
 */
void rasterizer_force_bilinear_filtering(void)
{

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        render_device().set_render_state(halo::d3d9::rs::lighting, 0);
    }

    render_device().set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(0, halo::d3d9::ss::max_anisotropy, 1);
    render_device().set_sampler_state(1, halo::d3d9::ss::max_anisotropy, 1);

    if (halo::d3d9::k_pixel_shader_version_1_0 < rasterizer_caps.pixel_shader_version) {
        render_device().set_sampler_state(2, halo::d3d9::ss::min_filter, 2);
        render_device().set_sampler_state(3, halo::d3d9::ss::min_filter, 2);
        render_device().set_sampler_state(2, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(3, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(2, halo::d3d9::ss::max_anisotropy, 1);
        render_device().set_sampler_state(3, halo::d3d9::ss::max_anisotropy, 1);
    }

    rasterizer_set_shader_stage_config(2);
}

}  // namespace rasterizer_force_bilinear_filtering_impl

namespace rasterizer_render_target_bind_effect_texture_impl {


/**
 * Binds a render-target texture to one of an effect's named texture handles instead of a raw device sampler
 * stage.
 *
 * Registers: AX -> target_index, EDX -> effect_slot, stack -> handle_index
 *
 * @address 0x52ce10
 */
void rasterizer_render_target_bind_effect_texture(int16_t target_index, rasterizer_effect_slot *effect_slot, int16_t handle_index)
{
    void *texture = 0;

    if (target_index < 9 && target_index > -1) {
        texture = rasterizer_render_targets[target_index].texture;
    }

    render_device().effect_set_texture(effect_slot->effect, effect_slot->texture_handles[handle_index], texture);
}

}  // namespace rasterizer_render_target_bind_effect_texture_impl

/**
 * Binds a previously rendered off-screen render target as a texture on the given sampler stage.
 *
 * Registers: AX -> target_index, DX -> stage
 *
 * @address 0x52cdd0
 */
void * rasterizer_render_target_bind_texture_stage(int16_t target_index, int16_t stage)
{
    void *texture = 0;

    if (target_index < 9 && target_index > -1) {
        texture = rasterizer_render_targets[target_index].texture;
    }

    render_device().set_texture(stage, texture);
    return texture;
}

/**
 * Direct3D 9 back end function rasterizer_resolve_and_cache_submap_b. The original author notes are in
 * docs/original/rasterizer/rasterizer_resolve_and_cache_submap_b.c.txt.
 *
 * Registers: EAX -> bitmap_tag_id, CX -> bitmap_type, stack -> (stage, default_index, frame, effect_slot)
 *
 * @address 0x518860
 */
int16_t * rasterizer_resolve_and_cache_submap_b(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage, int16_t default_index, int16_t frame, rasterizer_effect_slot *effect_slot)
{
    uint8_t resolved;
    BitmapData *data = rasterizer_tag_bitmap(bitmap_tag_id, bitmap_type, default_index, frame, &resolved);

    if (!resolved || *(int16_t *)&((struct BitmapData *)data)->type != bitmap_type) {
        data = rasterizer_default_bitmap(bitmap_type, default_index);
        if (data == 0) {
            return 0;
        }
    }
    rasterizer_bind_texture_d3dx(stage, data, effect_slot);
    rasterizer_bound_bitmap_size_b[0] = (int16_t)data->width;
    rasterizer_bound_bitmap_size_b[1] = (int16_t)data->height;
    return rasterizer_bound_bitmap_size_b;
}

/**
 * Direct3D 9 back end function rasterizer_resolve_and_cache_submap_c. The original author notes are in
 * docs/original/rasterizer/rasterizer_resolve_and_cache_submap_c.c.txt.
 *
 * Registers: EAX -> bitmap_tag_id, DI -> bitmap_type, stack -> (stage, default_index, frame)
 *
 * @address 0x518a60
 */
uint8_t rasterizer_resolve_and_cache_submap_c(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage, int16_t default_index, int16_t frame)
{
    uint8_t resolved;
    BitmapData *data = rasterizer_tag_bitmap(bitmap_tag_id, bitmap_type, default_index, frame, &resolved);

    if (resolved) {
        if (halo::cache::texture_cache_get(data, 0, 1) == 0) {
            return 1;
        }
    }
    if (!resolved || *(int16_t *)&((struct BitmapData *)data)->type != bitmap_type) {
        data = rasterizer_default_bitmap(bitmap_type, default_index);
        if (data == 0) {
            return 0;
        }
    }
    rasterizer_bind_texture_d3d9(stage, data);
    rasterizer_bound_bitmap_size_c[0] = (int16_t)data->width;
    rasterizer_bound_bitmap_size_c[1] = (int16_t)data->height;
    return 0;
}

namespace rasterizer_unbind_stream_and_textures_impl {




/**
 * Unbinds texture stages 0 and 1, clears the current stream source, and clears the current vertex
 * declaration/FVF.
 *
 * @address 0x518130
 */
void rasterizer_unbind_stream_and_textures(void)
{
    int32_t stage;

    for (stage = 0; stage < 2; stage++) {
        render_device().set_texture(stage, nullptr);
    }

    render_device().set_stream_source(0, nullptr, 0, 0);

    render_device().set_indices(0);
}

}  // namespace rasterizer_unbind_stream_and_textures_impl

/**
 * Direct3D 9 back end function rasterizer_validate_and_rebind_texture. The original author notes are in
 * docs/original/rasterizer/rasterizer_validate_and_rebind_texture.c.txt.
 *
 * Registers: EAX -> bitmap_tag_id, stack -> (stage, frame)
 *
 * @address 0x5187e0
 */
uint8_t rasterizer_validate_and_rebind_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t frame)
{
    BitmapData *data;

    if (bitmap_tag_id == halo::k_dword_none) {
        return 0;
    }
    data = bitmap_group_frame(bitmap_tag_id, frame);
    if (data == 0) {
        return 0;
    }
    if (halo::cache::texture_cache_get(data, 0, 1) == 0) {
        return 1;
    }
    rasterizer_bind_texture_d3d9(stage, data);
    return 0;
}

}  // namespace halo::rasterizer
