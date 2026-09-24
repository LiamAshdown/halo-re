// rasterizer_bitmap_upload_cubemap_mipmaps  (Ghidra: FUN_00524270)
// address 0x524270, size 322 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: out/phase2/results/rasterizer_01.json ("Uploads a packed cubemap bitmap's mip chain
// into a locked Direct3D cube texture, level by level."). Same LockRect/UnlockRect vtable shape
// (+0x4c/+0x50) as rasterizer_bitmap_upload_2d_mipmaps.c, but this one loops mip levels through
// `unaff_EBX + 0x14` gated by a capability bit (`(char)(DAT_007c10fc >> 8) < 0`, the same
// mipmappable-cube-map test used in rasterizer_bitmap_create_hardware_texture.c) and, for each
// level, loops cube faces (`bitmap_data_calculate_mip_depth`), computing each face's byte size
// from a pixel count times a bits-per-pixel table (`DAT_006571f4`, indexed by BitmapDataFormat_t)
// divided by 8 with a rounding correction. Given the register-only parameter (`unaff_EBX`, no
// stack parameters at all) and the bit-depth arithmetic's `CONCAT44`/64-bit-divide rendering,
// this rewrite preserves only the outer level/face loop shape and the two confirmed D3D calls;
// the per-face byte size computation is approximated rather than reproduced exactly. Treat this
// as a low-confidence structural placeholder.
// register convention: EBX = bitmap (live-in; no stack parameters).
// UNSURE: bitmap_data_get_pixel_address, bitmap_data_calculate_mip_depth and
// bitmap_data_calculate_mip_level_pixel_count are called with every argument elided by Ghidra;
// signatures inferred as (bitmap, mip_level) by analogy with rasterizer_bitmap_upload_2d_mipmaps.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <string.h>
// reconciled: the Direct3D texture is BitmapData.hardware_texture (+0x28, retail PC runtime); tags.h's `pointer` (+0x24) is a different field

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern void *rasterizer_device; // 0x0071d174
extern int8_t rasterizer_bitmap_format_bits_per_pixel[];            // 0x006571f4 indexed by BitmapDataFormat

extern void *bitmap_data_get_pixel_address(BitmapData *bitmap, int32_t mip_level); // 0x43fb20, UNSURE signature
extern int16_t bitmap_data_calculate_mip_depth(BitmapData *bitmap, int32_t mip_level); // 0x43fbe0, UNSURE signature
extern uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t mip_level); // 0x43fc10, UNSURE signature

typedef int32_t (__stdcall *d3d_lock_rect_fn)(void *self, uint32_t face, uint32_t level, void *out_rect, const void *rect, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_rect_fn)(void *self, uint32_t face, uint32_t level);


// blam-cc: EBX = bitmap
void rasterizer_bitmap_upload_cubemap_mipmaps(BitmapData *bitmap)
{
    uint8_t ok;
    int16_t max_level;
    int16_t level;
    int16_t face;
    int16_t face_count;
    void **vtable;
    d3d_lock_rect_fn lock_rect;
    d3d_unlock_rect_fn unlock_rect;
    d3d_locked_rect locked;
    int32_t hresult;
    uint8_t *source;
    uint8_t *dest;
    uint32_t pixel_count;
    uint32_t face_bytes;

    ok = 1;
    if (rasterizer_device == 0 || *(uint32_t *)((uint8_t *)bitmap + 0x2c) == 0 ||
        bitmap->hardware_texture == 0) {
        return;
    }

    max_level = ((int8_t)(rasterizer_caps.texture_caps >> 8) < 0) ? bitmap->mipmap_count : 0;

    for (level = 0; ok; level++) {
        if (max_level < level) {
            return;
        }

        vtable = *(void ***)(void *)bitmap->hardware_texture;
        lock_rect = (d3d_lock_rect_fn)vtable[0x13]; // +0x4c, UNSURE: 5-arg LockRect shape guessed for cube faces
        hresult = lock_rect((void *)bitmap->hardware_texture, 0, (uint32_t)level, &locked, 0, 0);
        if (hresult < 0 || locked.bits == 0) {
            ok = 0;
            break;
        }

        source = (uint8_t *)bitmap_data_get_pixel_address(bitmap, level);
        face_count = bitmap_data_calculate_mip_depth(bitmap, level);
        dest = (uint8_t *)locked.bits;
        for (face = 0; face < face_count; face++) {
            pixel_count = bitmap_data_calculate_mip_level_pixel_count(bitmap, level);
            face_bytes = (pixel_count * rasterizer_bitmap_format_bits_per_pixel[bitmap->format]) / 8; // UNSURE: rounding correction dropped
            memcpy(dest, source, face_bytes);
            source = source + face_bytes;
            dest = dest + locked.pitch; // UNSURE: original advances by a separate iStack_8 stride
        }

        vtable = *(void ***)(void *)bitmap->hardware_texture;
        unlock_rect = (d3d_unlock_rect_fn)vtable[0x14]; // +0x50
        hresult = unlock_rect((void *)bitmap->hardware_texture, 0, (uint32_t)level);
        if (hresult < 0) {
            ok = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x524270) -- see `python tools/pack.py 0x524270` for the full
body; this rewrite is a low-confidence structural placeholder, see file header.
#endif
