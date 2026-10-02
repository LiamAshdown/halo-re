// bitmap_data_get_pixel_address  (Ghidra: bitmap_data_get_pixel_address, already named)
// address 0x43fb20, size 80 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_functions.md summary ("Dispatches to the appropriate 2D/3D/
//   cubemap pixel-address routine based on a bitmap_data's type field") and
//   out/phase4/bitmaps_types_notes.md register table. Ghidra shows each callee invoked with all
//   x/y/z/face arguments as literal 0 (`bitmap_data_get_row_address(0,0)`,
//   `FUN_0043f990(0,0,0)`, `FUN_0043fa90(0,0,0)`), i.e. this only resolves the base address of
//   a whole mip level, not an arbitrary pixel; callers confirm this (e.g.
//   src/rasterizer/rasterizer_bitmap_upload_2d_mipmaps.c calls it as
//   bitmap_data_get_pixel_address(bitmap, source_mip) with just the two live arguments).
// register convention: ECX bitmap, EAX mip level, per bitmaps_types_notes.md.
//   // blam-cc: EAX -> mip_level, ECX -> bitmap
// NOTE: this function's own two arguments follow the task's mandated EAX/ECX/EDX/EBX/ESI/EDI-
//   then-stack ordering (mip_level before bitmap). Its sibling pixel-address routines in this
//   module (bitmap_data_get_row_address, bitmap_data_get_volume_pixel_address, both written by
//   another agent in this session) instead put the BitmapData pointer first; this file calls
//   them using their actual, now-confirmed signatures rather than the register-order rule, since
//   those signatures are already fixed by the files that define them.

#include "tags.h"
#include "bitmaps.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *bitmap_data_get_row_address(BitmapData *bitmap_data, int16_t mip_level, int16_t x, int16_t y); // 0x43f8e0, this module
extern void *bitmap_data_get_volume_pixel_address(BitmapData *bitmap_data, int16_t x, int16_t y, int16_t z, int16_t mip_level); // 0x43f990, this module
extern void *bitmap_data_get_cube_map_pixel_address(BitmapData *bitmap, int32_t mip_level, int16_t x, int16_t y, int16_t face); // 0x43fa90, this module

// blam-cc: EAX -> mip_level, ECX -> bitmap
// Returns the base address of bitmap's pixel data at mip_level, dispatching on the bitmap_data's
// type: 2D bitmaps use bitmap_data_get_row_address, 3D volumes use
// bitmap_data_get_volume_pixel_address, cube maps use bitmap_data_get_cube_map_pixel_address,
// all queried at (x, y, z/face) == (0, 0, 0). Any other type (bitmapdatatype_white) falls back
// to returning the BitmapData pointer itself, unchanged.
void *bitmap_data_get_pixel_address(BitmapData *bitmap, int32_t mip_level)
{
    switch (bitmap->type) {
    case bitmapdatatype_2d_texture:
        return bitmap_data_get_row_address(bitmap, (int16_t)mip_level, 0, 0);
    case bitmapdatatype_3d_texture:
        return bitmap_data_get_volume_pixel_address(bitmap, 0, 0, 0, (int16_t)mip_level);
    case bitmapdatatype_cube_map:
        return bitmap_data_get_cube_map_pixel_address(bitmap, mip_level, 0, 0, 0);
    default:
        return bitmap;
    }
}

#if 0
Original Ghidra decompilation (0x43fb20):

int bitmap_data_get_pixel_address(void)

{
  short sVar1;
  int iVar2;
  int in_ECX;

  sVar1 = *(short *)(in_ECX + 10);
  if (sVar1 == 0) {
    iVar2 = bitmap_data_get_row_address(0,0);
    return iVar2;
  }
  if (sVar1 != 1) {
    if (sVar1 == 2) {
      iVar2 = FUN_0043fa90(0,0,0);
      return iVar2;
    }
    return in_ECX;
  }
  iVar2 = FUN_0043f990(0,0,0);
  return iVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
