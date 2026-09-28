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
extern int8_t bitmap_format_bits_per_pixel[];            // 0x006571f4 indexed by BitmapDataFormat

extern void *bitmap_data_get_pixel_address(BitmapData *bitmap, int32_t mip_level); // 0x43fb20, UNSURE signature
extern int16_t bitmap_data_calculate_mip_depth(BitmapData *bitmap, int32_t mip_level); // 0x43fbe0, UNSURE signature
extern uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t mip_level); // 0x43fc10, UNSURE signature

// IDirect3DVolumeTexture9::LockBox (+0x4c) and UnlockBox (+0x50)
typedef struct d3d_locked_box {
    int32_t row_pitch;    // 0x00
    int32_t slice_pitch;  // 0x04
    void *bits;           // 0x08
} d3d_locked_box;
typedef int32_t (__stdcall *d3d_lock_box_fn)(void *self, uint32_t level, d3d_locked_box *out_box, const void *box, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_box_fn)(void *self, uint32_t level);

// REWRITTEN (objdump 0x524270..0x5243b7, 2026-09-25): this uploads a VOLUME (3D) texture, one depth slice at a
//   time. The draft called LockBox/UnlockBox with an extra argument (these are __stdcall: the stack drifted),
//   copied a whole level per slice instead of level/depth, and stepped the destination by the row pitch
//   instead of the slice pitch.
// blam-cc: EBX -> bitmap
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
    void **vtable;

    if (rasterizer_device == 0 || *(uint32_t *)&((struct BitmapData *)bitmap)->pixel_base == 0 || bitmap->hardware_texture == 0) {
        return;
    }
    // D3DPTEXTURECAPS_MIPVOLUMEMAP (TextureCaps bit 15): upload every mip level, else only the first
    max_level = ((int8_t)(rasterizer_caps.texture_caps >> 8) < 0) ? bitmap->mipmap_count : 0;

    for (level = 0; ok && level <= max_level; level++) {
        vtable = *(void ***)(void *)bitmap->hardware_texture;
        if (((d3d_lock_box_fn)vtable[0x4c / 4])((void *)bitmap->hardware_texture, (uint32_t)level, &locked, 0, 0) < 0 ||
            locked.bits == 0) {
            ok = 0;   // 0x52439b: a failed lock is not unlocked
            continue;
        }
        source = (uint8_t *)bitmap_data_get_pixel_address(bitmap, level);
        depth = bitmap_data_calculate_mip_depth(bitmap, level);
        dest = (uint8_t *)locked.bits;
        for (slice = 0; slice < depth; slice++) {
            // 0x524334: the level's bytes (pixels * bits per pixel / 8, rounded toward zero), split evenly by depth
            level_bytes = (int32_t)bitmap_data_calculate_mip_level_pixel_count(bitmap, level) *
                          bitmap_format_bits_per_pixel[bitmap->format];
            level_bytes = level_bytes / 8;
            slice_bytes = level_bytes / depth;
            memcpy(dest, source, slice_bytes);
            source += slice_bytes;
            dest += locked.slice_pitch;
        }
        vtable = *(void ***)(void *)bitmap->hardware_texture;
        if (((d3d_unlock_box_fn)vtable[0x50 / 4])((void *)bitmap->hardware_texture, (uint32_t)level) < 0) {
            ok = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x524270) -- see `python tools/pack.py 0x524270` for the full
body; this rewrite is a low-confidence structural placeholder, see file header.
#endif
