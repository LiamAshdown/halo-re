// rasterizer_bitmap_upload_cubemap_mipmaps_by_face  (Ghidra: FUN_005243c0)
// address 0x5243c0, size 456 bytes
// name confidence: 0.35   rewrite confidence: 0.15
// evidence: out/phase2/results/rasterizer_01.json ("Uploads all six faces of a packed cubemap
// bitmap for each mip level, an alternate iteration order to
// rasterizer_bitmap_upload_cubemap_mipmaps."). Same gating fields (bitmap+0x2c, bitmap->hardware_texture)
// and the same mipmappable-cube-map capability test (bit 0x10000 of texture_caps) as
// rasterizer_bitmap_upload_cubemap_mipmaps.c, but with bitmap_data_get_cube_map_pixel_address (a face-address helper, not
// decoded here) in place of bitmap_data_get_pixel_address, and iterating faces as the outer loop
// per its own phase2 summary. Given the size and register-only parameter, this rewrite preserves
// only the gating and the two confirmed D3D LockRect/UnlockRect calls in outline; the per-row
// copy arithmetic is approximated. Treat this as a low-confidence structural placeholder.
// register convention: param_1 as the recognized parameter (bitmap).
// UNSURE: bitmap_data_get_cube_map_pixel_address, bitmap_data_calculate_mip_dimension,
// bitmap_data_calculate_mip_level_pixel_count and bitmap_data_calculate_mip_row_byte_size are
// all called with every argument elided by Ghidra.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <string.h>
// reconciled: the Direct3D texture is BitmapData.hardware_texture (+0x28, retail PC runtime); tags.h's `pointer` (+0x24) is a different field

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern void *rasterizer_device; // 0x0071d174
extern int8_t rasterizer_bitmap_format_bits_per_pixel[];            // 0x006571f4 indexed by BitmapDataFormat

extern void *bitmap_data_get_cube_map_pixel_address(BitmapData *bitmap, int32_t face, int32_t mip_level); // 0x43fa90, UNSURE signature
extern uint32_t bitmap_data_calculate_mip_dimension(BitmapData *bitmap, int32_t mip_level); // 0x43fbb0
extern uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t mip_level); // 0x43fc10
extern uint32_t bitmap_data_calculate_mip_row_byte_size(BitmapData *bitmap, int32_t mip_level); // 0x43fce0

typedef int32_t (__stdcall *d3d_lock_rect_fn)(void *self, uint32_t face, uint32_t level, void *out_rect, const void *rect, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_rect_fn)(void *self, uint32_t face, uint32_t level);

// blam-cc: param_1 = bitmap
void rasterizer_bitmap_upload_cubemap_mipmaps_by_face(BitmapData *bitmap)
{
    uint8_t ok;
    int16_t max_level;
    int16_t face;
    int16_t level;
    void **vtable;
    d3d_lock_rect_fn lock_rect;
    d3d_unlock_rect_fn unlock_rect;
    d3d_locked_rect locked;
    int32_t hresult;
    uint8_t *source;
    uint8_t *dest;
    uint32_t row, rows, row_size;

    ok = 1;
    if (rasterizer_device == 0 || *(uint32_t *)((uint8_t *)bitmap + 0x2c) == 0 ||
        bitmap->hardware_texture == 0) {
        return;
    }

    max_level = ((rasterizer_caps.texture_caps & 0x10000) == 0) ? 0 : bitmap->mipmap_count;

    for (face = 0; face < 6 && ok; face++) {
        for (level = 0; level <= max_level && ok; level++) {
            vtable = *(void ***)(void *)bitmap->hardware_texture;
            lock_rect = (d3d_lock_rect_fn)vtable[0x13]; // +0x4c
            hresult = lock_rect((void *)bitmap->hardware_texture, (uint32_t)face, (uint32_t)level, &locked, 0, 0);
            if (hresult < 0 || locked.bits == 0) {
                ok = 0;
                break;
            }

            source = (uint8_t *)bitmap_data_get_cube_map_pixel_address(bitmap, face, level);
            rows = bitmap_data_calculate_mip_dimension(bitmap, level);
            row_size = bitmap_data_calculate_mip_row_byte_size(bitmap, level);
            dest = (uint8_t *)locked.bits;
            for (row = 0; row < rows; row++) {
                memcpy(dest, source, row_size);
                source = source + row_size;
                dest = dest + locked.pitch;
            }

            vtable = *(void ***)(void *)bitmap->hardware_texture;
            unlock_rect = (d3d_unlock_rect_fn)vtable[0x14]; // +0x50
            hresult = unlock_rect((void *)bitmap->hardware_texture, (uint32_t)face, (uint32_t)level);
            if (hresult < 0) {
                ok = 0;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x5243c0) -- see `python tools/pack.py 0x5243c0` for the full
body; this rewrite is a low-confidence structural placeholder, see file header.
#endif
