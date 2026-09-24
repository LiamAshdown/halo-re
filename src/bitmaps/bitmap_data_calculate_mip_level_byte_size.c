// bitmap_data_calculate_mip_level_byte_size  (Ghidra: bitmap_data_calculate_mip_level_byte_size,
// already named)
// address 0x43fcb0, size 33 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: calls bitmap_data_calculate_mip_level_pixel_count and converts its result from bits
//   (pixel_count * bitmap_format_bits_per_pixel[format]) to bytes; out/phase4/
//   bitmaps_functions.md: "Computes the byte size of a single mip level's pixel data, given its
//   pixel count and the bitmap's pixel format."
// register convention: EAX bitmap, per bitmaps_types_notes.md ("0x43fb70 / 0x43fcb0: EAX
//   BitmapData (0x43fcb0 also CL level)").
//   // blam-cc: EAX -> bitmap, ECX (CL) -> level
// The bits-to-bytes step below is plain C division (`bits / 8`) rather than Ghidra's
// `(bits + (bits >> 0x1f & 7)) >> 3`; that expression is the textbook lowering of signed integer
// division by a power of two (add the bias only when negative, then arithmetic-shift), and the
// two were checked to agree for the full int32 range.

#include "tags.h"
#include "bitmaps.h"

extern int8_t bitmap_format_bits_per_pixel[k_bitmap_data_format_count]; // 0x006571f4 (types/bitmaps.h)
extern uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t level); // 0x43fc10, this module

// blam-cc: EAX -> bitmap, ECX (CL) -> level
// Computes the byte size of bitmap's pixel data at mip level `level`.
uint32_t bitmap_data_calculate_mip_level_byte_size(BitmapData *bitmap, int32_t level)
{
    int32_t pixel_count;
    int32_t bits;

    pixel_count = (int32_t)bitmap_data_calculate_mip_level_pixel_count(bitmap, level);
    bits = pixel_count * (int32_t)bitmap_format_bits_per_pixel[bitmap->format];
    return (uint32_t)(bits / 8);
}

#if 0
Original Ghidra decompilation (0x43fcb0):

int bitmap_data_calculate_mip_level_byte_size(void)

{
  int in_EAX;
  int iVar1;

  iVar1 = bitmap_data_calculate_mip_level_pixel_count();
  return (int)(iVar1 * (char)(&DAT_006571f4)[*(short *)(in_EAX + 0xc)] +
              (iVar1 * (char)(&DAT_006571f4)[*(short *)(in_EAX + 0xc)] >> 0x1f & 7U)) >> 3;
}
#endif
