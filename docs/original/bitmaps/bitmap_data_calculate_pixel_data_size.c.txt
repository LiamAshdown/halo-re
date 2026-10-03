// bitmap_data_calculate_pixel_data_size  (Ghidra: bitmap_data_calculate_pixel_data_size, already
// named)
// address 0x43fb70, size 62 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: sums bitmap_data_calculate_mip_level_pixel_count over every level from 0 to
//   BitmapData::mipmap_count inclusive, converts the total from bits to bytes; out/phase4/
//   bitmaps_functions.md: "Computes the total byte size of a bitmap_data's pixel data across all
//   its mip levels." src/main/screenshot_render.c already calls this (as
//   bitmap_data_calculate_pixel_data_size(bitmap)) to size a GlobalAlloc for a runtime bitmap.
// register convention: EAX bitmap, per bitmaps_types_notes.md ("0x43fb70 / 0x43fcb0: EAX
//   BitmapData ...").
//   // blam-cc: EAX -> bitmap
// The bits-to-bytes step below is plain C division (`bits / 8`) rather than Ghidra's
// `(bits + (bits >> 0x1f & 7)) >> 3`; that expression is the textbook lowering of signed integer
// division by a power of two (add the bias only when negative, then arithmetic-shift), and the
// two were checked to agree for the full int32 range.

#include "tags.h"
#include "bitmaps.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int8_t bitmap_format_bits_per_pixel[k_bitmap_data_format_count]; // 0x006571f4 (types/bitmaps.h)
extern uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t level); // 0x43fc10, this module

// blam-cc: EAX -> bitmap
// Computes the total byte size of bitmap's pixel data across every mip level (0 through
// mipmap_count inclusive).
uint32_t bitmap_data_calculate_pixel_data_size(BitmapData *bitmap)
{
    int32_t total_pixels;
    int16_t level;
    int32_t bits;

    total_pixels = 0;
    if (0 <= (int16_t)bitmap->mipmap_count) {
        level = 0;
        do {
            total_pixels += (int32_t)bitmap_data_calculate_mip_level_pixel_count(bitmap, level);
            level++;
        } while (level <= (int16_t)bitmap->mipmap_count);
    }

    bits = total_pixels * (int32_t)bitmap_format_bits_per_pixel[bitmap->format];
    return (uint32_t)(bits / 8);
}

#if 0
Original Ghidra decompilation (0x43fb70):

int bitmap_data_calculate_pixel_data_size(void)

{
  short sVar1;
  int in_EAX;
  int iVar2;
  int iVar3;
  short sVar4;

  sVar1 = *(short *)(in_EAX + 0x14);
  iVar3 = 0;
  sVar4 = 0;
  if (-1 < sVar1) {
    do {
      iVar2 = bitmap_data_calculate_mip_level_pixel_count();
      iVar3 = iVar3 + iVar2;
      sVar4 = sVar4 + 1;
    } while (sVar4 <= sVar1);
  }
  return (int)((char)(&DAT_006571f4)[*(short *)(in_EAX + 0xc)] * iVar3 +
              ((char)(&DAT_006571f4)[*(short *)(in_EAX + 0xc)] * iVar3 >> 0x1f & 7U)) >> 3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
