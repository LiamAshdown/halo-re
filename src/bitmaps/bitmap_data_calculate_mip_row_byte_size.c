// bitmap_data_calculate_mip_row_byte_size  (Ghidra: bitmap_data_calculate_mip_row_byte_size,
// already named)
// address 0x43fce0, size 75 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: computes width at the mip level exactly like bitmap_data_calculate_mip_dimension
//   (>>= level, clamped to 1, rounded up to a multiple of 4 when compressed), multiplies by
//   bitmap_format_bits_per_pixel[format] and converts bits to bytes; out/phase4/
//   bitmaps_functions.md: "Computes the byte pitch (row size) of a bitmap_data at a given mip
//   level for its pixel format."
// register convention: EDX bitmap, CL level, per bitmaps_types_notes.md ("0x43fbb0 / 0x43fce0:
//   EDX BitmapData, CL level").
//   // blam-cc: ECX (CL) -> level, EDX -> bitmap
// The bits-to-bytes step below is plain C division (`bits / 8`) rather than Ghidra's
// `(bits + (bits >> 0x1f & 7)) >> 3`; that expression is the textbook lowering of signed integer
// division by a power of two (add the bias only when negative, then arithmetic-shift), and the
// two were checked to agree for the full int32 range.
// The round-up-to-a-multiple-of-4 step is `(width + 3) & ~3` rather than Ghidra's
// `width + ((byte)-(char)width & 3)`; checked to agree for every width in [0, 40000].

#include "tags.h"
#include "bitmaps.h"

extern int8_t bitmap_format_bits_per_pixel[k_bitmap_data_format_count]; // 0x006571f4 (types/bitmaps.h)

// blam-cc: ECX (CL) -> level, EDX -> bitmap
// Computes the byte pitch of one row of bitmap's pixels at mip level `level`.
uint32_t bitmap_data_calculate_mip_row_byte_size(BitmapData *bitmap, int32_t level)
{
    int32_t width;
    int32_t bits;

    width = (int16_t)bitmap->width >> (level & 0x1f);
    if (width < 2) {
        width = 1;
    }
    if ((bitmap->flags & _bitmap_data_compressed_bit) != 0) {
        width = (width + 3) & ~3;
    }

    bits = (int32_t)bitmap_format_bits_per_pixel[bitmap->format] * width;
    return (uint32_t)(bits / 8);
}

#if 0
Original Ghidra decompilation (0x43fce0):

int bitmap_data_calculate_mip_row_byte_size(void)

{
  short sVar1;
  byte in_CL;
  int in_EDX;

  if (*(short *)(in_EDX + 4) >> (in_CL & 0x1f) < 2) {
    sVar1 = 1;
  }
  else {
    sVar1 = *(short *)(in_EDX + 4) >> (in_CL & 0x1f);
  }
  if ((*(byte *)(in_EDX + 0xe) & 2) != 0) {
    sVar1 = sVar1 + ((byte)-(char)sVar1 & 3);
  }
  return (int)((int)(char)(&DAT_006571f4)[*(short *)(in_EDX + 0xc)] * (int)sVar1 +
              ((int)(char)(&DAT_006571f4)[*(short *)(in_EDX + 0xc)] * (int)sVar1 >> 0x1f & 7U)) >> 3
  ;
}
#endif
