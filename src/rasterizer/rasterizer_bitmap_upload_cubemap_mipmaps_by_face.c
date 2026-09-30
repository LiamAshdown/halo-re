// rasterizer_bitmap_upload_cubemap_mipmaps_by_face  (Ghidra: FUN_005243c0)
// address 0x5243c0, size 456 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (rewritten 2026-09-25 from the disassembly)
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
#include "fn_rasterizer.h"
#include "fn_bitmaps.h"
#include <string.h>
// reconciled: the Direct3D texture is BitmapData.hardware_texture (+0x28, retail PC runtime); tags.h's `pointer` (+0x24) is a different field

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern void *rasterizer_device; // 0x0071d174
extern int8_t bitmap_format_bits_per_pixel[];            // 0x006571f4 indexed by BitmapDataFormat


extern uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t mip_level); // 0x43fc10

extern int16_t rasterizer_cube_face_to_d3d_face[6]; // 0x0065e088: 0, 2, 1, 3, 4, 5 (Halo face order -> D3DCUBEMAP_FACES)

typedef int32_t (__stdcall *d3d_lock_rect_fn)(void *self, uint32_t face, uint32_t level, void *out_rect, const void *rect, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_rect_fn)(void *self, uint32_t face, uint32_t level);

// REWRITTEN (objdump 0x5243c0..0x52458a, 2026-09-25). For each mip level (every level only if the card reports
//   D3DPTEXTURECAPS_MIPCUBEMAP, TextureCaps bit 16) and each of the six faces: LockRect(D3D face, level), copy the
//   face (as one block of level bytes / 6 for compressed formats, else row by row at the locked pitch), UnlockRect.
//   The draft passed the pixel-address helper (face, level) instead of (level, 0, 0, face), used Halo's face index
//   directly instead of mapping it through 0x65e088, and approximated the copy sizes.
// blam-cc: stack -> bitmap
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
    void **vtable;

    if (rasterizer_device == 0 || *(uint32_t *)&((struct BitmapData *)bitmap)->pixel_base == 0 || bitmap->hardware_texture == 0) {
        return;
    }
    max_level = (rasterizer_caps.texture_caps & 0x10000) != 0 ? bitmap->mipmap_count : 0;

    for (level = 0; ok && level <= max_level; level++) {
        for (face = 0; ok && face < 6; face++) {
            vtable = *(void ***)(void *)bitmap->hardware_texture;
            if (((d3d_lock_rect_fn)vtable[0x4c / 4])((void *)bitmap->hardware_texture,
                    (uint32_t)rasterizer_cube_face_to_d3d_face[face], (uint32_t)level, &locked, 0, 0) < 0 ||
                locked.bits == 0) {
                ok = 0;   // 0x52455c: a failed lock is not unlocked
                continue;
            }
            dest = (uint8_t *)locked.bits;
            source = (uint8_t *)bitmap_data_get_cube_map_pixel_address(bitmap, level, 0, 0, face);
            if ((bitmap->flags & 2) != 0) {
                // compressed: the level's bytes (pixels * bits per pixel / 8, toward zero) split over six faces
                bytes = (int32_t)bitmap_data_calculate_mip_level_pixel_count(bitmap, level) *
                        bitmap_format_bits_per_pixel[bitmap->format];
                bytes = bytes / 8;
                memcpy(dest, source, bytes / 6);
            } else {
                rows = (int16_t)bitmap_data_calculate_mip_dimension(bitmap, level);
                row_size = bitmap_data_calculate_mip_row_byte_size(bitmap, level);
                for (row = 0; row < rows; row++) {
                    memcpy(dest, source, row_size);
                    source += row_size;
                    dest += locked.pitch;
                }
            }
            vtable = *(void ***)(void *)bitmap->hardware_texture;
            if (((d3d_unlock_rect_fn)vtable[0x50 / 4])((void *)bitmap->hardware_texture,
                    (uint32_t)rasterizer_cube_face_to_d3d_face[face], (uint32_t)level) < 0) {
                ok = 0;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x5243c0) -- see `python tools/pack.py 0x5243c0` for the full
body; this rewrite is a low-confidence structural placeholder, see file header.
#endif
