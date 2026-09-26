// rasterizer_bitmap_sample_texel  (Ghidra: FUN_00524590)
// address 0x524590, size 983 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN (objdump 0x524590..0x524966, format tables 0x524968 / 0x006571f4; the draft read the lock through
//   registers nobody set and decoded blocks with invented arguments). cdecl (bitmap, uv, mip_bias); returns the
//   texel as A8R8G8B8, or -1 without a hardware texture (+0x28), when the lock fails or for an unknown format.
//   The mip level is lrint((1 - bias) * mip count remaining) when bias < 1, there are mips left after the skip
//   (rasterizer_bitmap_compute_mipmap_skip_count, EAX bitmap, EBX/stack two words it fills) and nothing was
//   skipped; else 0. The level's width/height (at least 1, rounded up to 4 for compressed formats, then shifted
//   by the skip) turn uv into texel coordinates (lrint of the float size * uv - 0.5, wrapped with a mask for a
//   power of two, else a double modulo). The level is locked (IDirect3DTexture8::LockRect, vtable +0x4c, flags
//   0x810, read-only); no bits returns the width (as the binary does, without unlocking). Compressed formats read
//   the 4x4 block (bits per pixel from 0x006571f4) with the DXT1/DXT3/DXT5 texel decoders; uncompressed ones
//   expand R5G6B5 / A1R5G5B5 / A4R4G4B4 to A8R8G8B8 or read 32 or 8 bits (the 8-bit case indexes with a 4-byte
//   stride, as the binary does). UnlockRect (vtable +0x50) then returns the texel.
// blam-cc: stack -> bitmap, uv, mip_bias (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern int32_t rasterizer_bitmap_compute_mipmap_skip_count(BitmapData *bitmap, int16_t *out_width, int16_t *out_height);
    // 0x523f10, blam-cc: EAX, EBX, stack
extern void dxt1_decode_block_texel(void *out, void *block, int32_t x, int32_t y); // 0x43ffe0, EAX, stack
extern void dxt3_decode_alpha_texel(int32_t x, int32_t y, void *texel_out, void *block); // 0x440150, BL, SI, EDI, stack
extern void dxt5_decode_alpha_texel(void *block, void *texel_out, int32_t x, int32_t y); // 0x440190, EBX, stack
extern int8_t rasterizer_bitmap_format_bits_per_pixel[]; // 0x006571f4, indexed by format
extern long lrint(double x);

typedef struct locked_rect {
    int32_t pitch;
    uint8_t *bits;
} locked_rect;

static int32_t sample_texel_coordinate(int32_t size, float uv)
{
    float scaled = (float)size * uv - 0.5f;
    int32_t texel = (int32_t)lrint((double)scaled);

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

int32_t rasterizer_bitmap_sample_texel(BitmapData *bitmap, float *uv, float mip_bias)
{
    uint8_t *data = (uint8_t *)bitmap;
    void *texture = *(void **)(data + 0x28);
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
    remaining = (int16_t)(*(int16_t *)(data + 0x14) - skip);
    level = 0;
    if (mip_bias < 1.0f && remaining > 0 && skip == 0) {
        float mip = (1.0f - mip_bias) * (float)remaining;

        level = (int16_t)lrint((double)mip);
    }
    compressed = (uint8_t)((*(uint16_t *)(data + 0xe) & 2) != 0);
    width = level_dimension(*(uint16_t *)(data + 0x4), level, skip, compressed);
    height = level_dimension(*(uint16_t *)(data + 0x6), level, skip, compressed);
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
        int16_t format = *(int16_t *)(data + 0xc);
        int32_t block_bytes = ((int32_t)rasterizer_bitmap_format_bits_per_pixel[format] * 16) / 8;
        int32_t block_row = (int16_t)(y / 4);
        int32_t block_index = (block_row * width) / 4 + (int16_t)(x / 4);
        uint8_t *block = locked.bits + block_index * (int16_t)block_bytes;

        x &= 3;
        y &= 3;
        switch (format) {
        case 0xe: dxt1_decode_block_texel(&texel, block, x, y); break;
        case 0xf: dxt3_decode_alpha_texel(x, y, &texel, block); break;
        case 0x10: dxt5_decode_alpha_texel(block, &texel, x, y); break;
        default: break;
        }
    } else {
        uint8_t *row = locked.bits + y * locked.pitch;
        uint32_t v;
        uint32_t a;
        uint32_t b;
        uint32_t c;
        uint32_t d;

        switch (*(int16_t *)(data + 0xc)) {
        case 6: // R5G6B5
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
        case 8: // A1R5G5B5
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
        case 9: // A4R4G4B4
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

#if 0
Original Ghidra decompilation (0x524590) -- see `python tools/pack.py 0x524590` for the full
983-byte body; this rewrite is a lower-confidence structural approximation past the mip/UV
selection stage, see file header.
#endif
