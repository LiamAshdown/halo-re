// bitmap_data_depth_valid_for_type  (Ghidra: FUN_0043fe30; named here, not yet renamed in
// Ghidra/CEA)
// address 0x43fe30, size 34 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_types_notes.md, "Misnamed or misattributed functions": "0x43fd30
//   does not validate a cache chunk header ... It is bitmap_data_verify(bitmap,
//   require_runtime). 0x43fe30 is its depth/type check." Its own decompile: valid when
//   0 < depth <= k_bitmap_maximum_depth (256) and either depth == 1 or the bitmap is a
//   3D texture (BitmapDataType_t bitmapdatatype_3d_texture == 1) -- i.e. only 3D volumes may
//   have depth greater than 1.
// register convention: AX depth, stack type, per bitmaps_types_notes.md ("0x43fe30: AX depth,
//   stack type").
//   // blam-cc: EAX (AX) -> depth, stack -> type

#include "tags.h"
#include "bitmaps.h"

// blam-cc: EAX (AX) -> depth, stack -> type
// Validates a BitmapData's depth against its type: depth must be in (0, k_bitmap_maximum_depth],
// and any depth greater than 1 is only legal for a 3D texture.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint32_t bitmap_data_depth_valid_for_type(int32_t depth, BitmapDataType_t type)
{
    if (depth > 0 && depth <= k_bitmap_maximum_depth &&
        (depth == 1 || type == bitmapdatatype_3d_texture)) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x43fe30):

undefined4 FUN_0043fe30(short param_1)

{
  short in_AX;

  if (((0 < in_AX) && (in_AX < 0x101)) && ((in_AX == 1 || (param_1 == 1)))) {
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
