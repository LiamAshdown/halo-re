// bitmap_data_calculate_mip_depth  (Ghidra: bitmap_data_calculate_mip_depth, already named)
// address 0x43fbe0, size 34 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: reads BitmapData::depth (+0x08) right-shifted by the level, clamped to a minimum of
//   1 (no block-compressed rounding, unlike width/height -- depth is a slice count, not a pixel
//   dimension); out/phase4/bitmaps_functions.md: "Computes a bitmap's depth ... at a given mip
//   level, clamped to at least 1."
// register convention: EAX bitmap, DL level, per bitmaps_types_notes.md ("0x43fbe0: EAX
//   BitmapData, DL level").
//   // blam-cc: EAX -> bitmap, EDX (DL) -> level

#include "tags.h"
#include "bitmaps.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EAX -> bitmap, EDX (DL) -> level
// Computes bitmap's depth (volume slice count) at mip level `level`, clamped to a minimum of 1.
int32_t bitmap_data_calculate_mip_depth(BitmapData *bitmap, int32_t level)
{
    int32_t depth;

    depth = (int16_t)bitmap->depth >> (level & 0x1f);
    if (depth < 2) {
        depth = 1;
    }
    return depth;
}

#if 0
Original Ghidra decompilation (0x43fbe0):

int bitmap_data_calculate_mip_depth(void)

{
  int in_EAX;
  byte in_DL;

  if (1 < *(short *)(in_EAX + 8) >> (in_DL & 0x1f)) {
    return (int)*(short *)(in_EAX + 8) >> (in_DL & 0x1f);
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
