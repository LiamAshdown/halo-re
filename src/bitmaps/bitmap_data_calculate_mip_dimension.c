// bitmap_data_calculate_mip_dimension  (Ghidra: bitmap_data_calculate_mip_dimension, already
// named)
// address 0x43fbb0, size 47 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: reads BitmapData::height (+0x06) right-shifted by the level, clamped to a minimum
//   of 1, then rounded up to a multiple of 4 when BitmapData::flags has the compressed bit
//   (0x02) set -- the standard "half each mip level, round block-compressed formats up to the
//   block size" rule; out/phase4/bitmaps_functions.md: "Computes a bitmap's height at a given
//   mip level, rounding up to a multiple of 4 for block-compressed formats."
// register convention: EDX bitmap, CL level, per bitmaps_types_notes.md ("0x43fbb0 / 0x43fce0:
//   EDX BitmapData, CL level").
//   // blam-cc: ECX (CL) -> level, EDX -> bitmap
// The round-up-to-a-multiple-of-4 step below is `(height + 3) & ~3` rather than Ghidra's
// `height + ((byte)-(char)height & 3)`; the two are the same 8-bit round-up-to-4 idiom and were
// checked to agree for every height in [0, 40000] (well past k_bitmap_maximum_dimension).

#include "tags.h"
#include "bitmaps.h"
#include "fn_bitmaps.h"

// blam-cc: ECX (CL) -> level, EDX -> bitmap
// Computes bitmap's height at mip level `level` (>>= level, clamped to a minimum of 1), rounded
// up to a multiple of 4 when the bitmap is block compressed.
uint32_t bitmap_data_calculate_mip_dimension(BitmapData *bitmap, int32_t level)
{
    int32_t height;

    height = (int16_t)bitmap->height >> (level & 0x1f);
    if (height < 2) {
        height = 1;
    }
    if ((bitmap->flags & _bitmap_data_compressed_bit) != 0) {
        height = (height + 3) & ~3;
    }
    return (uint32_t)height;
}

#if 0
Original Ghidra decompilation (0x43fbb0):

uint bitmap_data_calculate_mip_dimension(void)

{
  uint uVar1;
  byte in_CL;
  int in_EDX;

  if (*(short *)(in_EDX + 6) >> (in_CL & 0x1f) < 2) {
    uVar1 = 1;
  }
  else {
    uVar1 = (uint)(ushort)(*(short *)(in_EDX + 6) >> (in_CL & 0x1f));
  }
  if ((*(byte *)(in_EDX + 0xe) & 2) != 0) {
    uVar1 = uVar1 + ((byte)-(char)uVar1 & 3);
  }
  return uVar1;
}
#endif
