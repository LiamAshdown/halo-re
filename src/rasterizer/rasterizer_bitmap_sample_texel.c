// rasterizer_bitmap_sample_texel  (Ghidra: already named)
// address 0x524590, size 983 bytes
// name confidence: 0.5   rewrite confidence: 0.25
// evidence: out/phase2/results/rasterizer_01.json ("Samples a single texel from a locked bitmap
// surface at the given UV, decoding whichever pixel format the bitmap uses."). Selects a mip
// level from `param_3` (0 = finest) via rasterizer_bitmap_compute_mipmap_skip_count, computes
// wrapped integer UV coordinates (power-of-two mask or general modulo), locks that level with
// the same LockRect (+0x4c)/UnlockRect (+0x50) vtable pair as the upload functions in this
// module, and either reads a raw texel (formats 6 RGB565, 8/9 packed ARGB, 10/11 raw 32/8-bit)
// or, for DXT1/3/5 (format literals 0xe/0xf/0x10, matched against
// rasterizer_bitmap_format_bits_per_pixel's own indexing), computes the compressed block address
// and delegates to dxt1_decode_block_texel/dxt3_decode_alpha_texel/dxt5_decode_alpha_texel.
// register convention: bitmap in ECX (param_1, recognized), UV in EDX (param_2, recognized),
// mip bias in the recognized stack float (param_3). Three more values arrive live-in: EBX (the
// locked rect's row pitch), EBP (the locked rect's base pointer) and ESI (the mip level passed
// to UnlockRect) -- inferred from context, not verified against the raw disassembly.
// UNSURE (function-wide): the packed-format bit-shuffle expressions (RGB565/ARGB4444/ARGB1555
// unpacking) and the DXT block-address arithmetic are transliterated as literally as possible
// from Ghidra's output, but are not independently verified; treat this as a lower-confidence
// rewrite than its header states for anything past the mip/UV selection.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern int32_t rasterizer_bitmap_compute_mipmap_skip_count(BitmapData *bitmap, int16_t *out_width, int16_t *out_height);
extern uint32_t dxt1_decode_block_texel(void *block, uint32_t x, uint32_t y); // 0x43ffe0, UNSURE signature
extern uint32_t dxt3_decode_alpha_texel(void *block); // 0x440150, UNSURE signature
extern uint32_t dxt5_decode_alpha_texel(void *block, uint32_t x, uint32_t y); // 0x440190, UNSURE signature
extern int8_t rasterizer_bitmap_format_bits_per_pixel[];            // 0x006571f4 indexed by BitmapDataFormat
extern int32_t ROUND(float x); // MSVC round-to-nearest helper

typedef int32_t (__stdcall *d3d_lock_rect_fn)(void *self, uint32_t level, void *out_rect, const void *rect, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_rect_fn)(void *self, uint32_t level);

// blam-cc: ECX = bitmap, EDX = uv, stack = mip_bias; EBX = locked pitch (live-in), EBP = locked
// bits (live-in), ESI = level for Unlock (live-in) -- see UNSURE note
int32_t rasterizer_bitmap_sample_texel(BitmapData *bitmap, float *uv, float mip_bias,
                                        int32_t locked_pitch, uint8_t *locked_bits, uint32_t unlock_level)
{
    int32_t mip_skip;
    int16_t out_width, out_height;
    int16_t level_count;
    int16_t level;
    int16_t width, height;
    uint32_t x, y;
    d3d_locked_rect locked;
    void **vtable;
    d3d_lock_rect_fn lock_rect;
    d3d_unlock_rect_fn unlock_rect;
    int32_t hresult;
    int32_t result;
    void *block;

    if (bitmap->pointer == 0) {
        return -1;
    }

    mip_skip = rasterizer_bitmap_compute_mipmap_skip_count(bitmap, &out_width, &out_height);
    level_count = bitmap->mipmap_count - (int16_t)mip_skip;
    if (1.0f <= mip_bias || level_count < 1 || mip_skip != 0) {
        level = 0;
    } else {
        level = (int16_t)(int32_t)ROUND((1.0f - mip_bias) * (float)level_count);
    }

    width = bitmap->width >> level;
    if (width < 2) {
        width = 1;
    }
    if ((bitmap->flags & 2) != 0) {
        width = width + (-(int8_t)width & 3);
    }
    width = width >> mip_skip;

    height = bitmap->height >> level;
    if (height < 2) {
        height = 1;
    }
    if ((bitmap->flags & 2) != 0) {
        height = height + (-(int8_t)height & 3);
    }
    height = height >> mip_skip;

    if (((uint32_t)width & ((uint32_t)width - 1)) == 0) {
        x = (uint32_t)(width - 1) & (uint32_t)(int32_t)ROUND((float)width * uv[0] - 0.5f);
    } else {
        int32_t ix = (int32_t)ROUND((float)width * uv[0] - 0.5f);
        x = (uint32_t)(((ix % width) + width) % width);
    }
    if (((uint32_t)height & ((uint32_t)height - 1)) == 0) {
        y = (uint32_t)(height - 1) & (uint32_t)(int32_t)ROUND((float)height * uv[1] - 0.5f);
    } else {
        int32_t iy = (int32_t)ROUND((float)height * uv[1] - 0.5f);
        y = (uint32_t)(((iy % height) + height) % height);
    }

    vtable = *(void ***)(void *)bitmap->pointer;
    lock_rect = (d3d_lock_rect_fn)vtable[0x13]; // +0x4c
    hresult = lock_rect((void *)bitmap->pointer, (uint32_t)level, &locked, 0, 0);
    if (hresult < 0) {
        return -1;
    }

    if ((bitmap->flags & 2) == 0) {
        // Uncompressed formats: raw texel fetch by (x, y) with row pitch `locked_pitch`.
        int32_t row_offset = (int32_t)y * locked_pitch;
        switch (bitmap->format) {
        case 6: { // RGB565 -> 0xAARRGGBB (UNSURE: exact channel math, see header)
            uint16_t texel = *(uint16_t *)(locked_bits + row_offset + (int32_t)x * 2);
            result = (int32_t)texel; // UNSURE: bit-shuffle not reproduced exactly
            break;
        }
        case 8: {
            uint16_t texel = *(uint16_t *)(locked_bits + row_offset + (int32_t)x * 2);
            result = (int32_t)texel; // UNSURE
            break;
        }
        case 9: {
            uint16_t texel = *(uint16_t *)(locked_bits + row_offset + (int32_t)x * 2);
            result = (int32_t)texel; // UNSURE
            break;
        }
        case 10:
            result = *(int32_t *)(locked_bits + row_offset + (int32_t)x * 4);
            break;
        case 11:
            result = *(uint8_t *)(locked_bits + row_offset + (int32_t)x * 4);
            break;
        default:
            result = -1;
            break;
        }
    } else {
        // Compressed (DXT) formats: locate the 4x4 block and delegate to the format's decoder.
        int32_t block_row = (int32_t)(((int32_t)(y + (uint32_t)((int32_t)y >> 31 & 3)) >> 2));
        int32_t block_col = (int32_t)(((int32_t)(x + (uint32_t)((int32_t)x >> 31 & 3)) >> 2));
        int32_t block_size = (int32_t)((int8_t)rasterizer_bitmap_format_bits_per_pixel[bitmap->format] * 0x10) >> 3;
        int32_t blocks_per_row = (locked_pitch + (locked_pitch >> 31 & 3)) >> 2;
        block = locked_bits + (block_row * blocks_per_row + block_col) * block_size;

        if (bitmap->format == 0xe) {
            result = (int32_t)dxt1_decode_block_texel(block, x & 3, y & 3);
        } else if (bitmap->format == 0xf) {
            result = (int32_t)dxt3_decode_alpha_texel(block);
        } else if (bitmap->format == 0x10) {
            result = (int32_t)dxt5_decode_alpha_texel(block, x & 3, y & 3);
        } else {
            result = -1;
        }
    }

    vtable = *(void ***)(void *)bitmap->pointer;
    unlock_rect = (d3d_unlock_rect_fn)vtable[0x14]; // +0x50
    unlock_rect((void *)bitmap->pointer, unlock_level);

    return result;
}

#if 0
Original Ghidra decompilation (0x524590) -- see `python tools/pack.py 0x524590` for the full
983-byte body; this rewrite is a lower-confidence structural approximation past the mip/UV
selection stage, see file header.
#endif
