// bitmap_data_get_row_address  (Ghidra: bitmap_data_get_row_address, already named)
// address 0x43f8e0, size 165 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_functions.md ("Computes the byte address of pixel (param_1,
// param_2) within a 2D bitmap_data's pixel buffer, accounting for bits-per-pixel.");
// types/bitmaps.h bitmap_format_bits_per_pixel (0x006571f4), bitmap_data_flags
// (_bitmap_data_compressed_bit), k_bitmap_compressed_block_dimension. The mip width/height
// walk is inlined here exactly as Ghidra shows it (not a call to a shared mip-dimension
// helper); every prior mip level's full width*height pixel count is summed to skip past it in
// the packed mip chain before adding this level's (x, y) offset.
// register convention: EDI = BitmapData *bitmap_data, AX = int16_t mip_level,
// stack -> int16_t x, int16_t y.
//   // blam-cc: EDI -> bitmap_data, EAX (low half, AX) -> mip_level, stack -> x, y

#include "tags.h"
#include "memory.h"
#include "bitmaps.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int8_t bitmap_format_bits_per_pixel[k_bitmap_data_format_count]; // 0x006571f4 (types/bitmaps.h)

// blam-cc: EDI -> bitmap_data, EAX (low half, AX) -> mip_level, stack -> x, y
void *bitmap_data_get_row_address(BitmapData *bitmap_data, int16_t mip_level, int16_t x, int16_t y)
{
    int16_t width = (int16_t)bitmap_data->width;
    int16_t height = (int16_t)bitmap_data->height;
    int16_t min_dimension = (bitmap_data->flags & _bitmap_data_compressed_bit) ?
        k_bitmap_compressed_block_dimension : 1;
    int32_t pixel_offset = 0;
    int32_t bit_offset;
    int16_t level;
    uint32_t base;

    for (level = mip_level; level > 0; level--) {
        pixel_offset += (int32_t)width * (int32_t)height;
        width  = (width  >> 1 >= min_dimension) ? (int16_t)(width  >> 1) : min_dimension;
        height = (height >> 1 >= min_dimension) ? (int16_t)(height >> 1) : min_dimension;
    }

    pixel_offset += (int32_t)x + (int32_t)width * (int32_t)y;
    bit_offset = pixel_offset * (int32_t)bitmap_format_bits_per_pixel[bitmap_data->format];

    base = (uint32_t)bitmap_data->pixel_base;
    return (void *)(base + (uint32_t)((bit_offset + ((bit_offset >> 31) & 7)) >> 3));
}

#if 0
Original Ghidra decompilation (0x43f8e0):

int bitmap_data_get_row_address(short param_1,short param_2)

{
  ushort in_AX;
  short sVar1;
  ushort uVar2;
  uint uVar3;
  short sVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  int unaff_EDI;
  uint local_c;

  uVar5 = (uint)*(ushort *)(unaff_EDI + 6);
  iVar7 = 0;
  local_c = (uint)in_AX;
  uVar2 = *(ushort *)(unaff_EDI + 4);
  uVar3 = (uint)uVar2;
  uVar6 = (-(uint)((*(byte *)(unaff_EDI + 0xe) & 2) != 0) & 3) + 1;
  if (0 < (short)in_AX) {
    do {
      sVar1 = (short)uVar3;
      sVar4 = (short)uVar5;
      iVar7 = iVar7 + (int)sVar1 * (int)sVar4;
      uVar3 = uVar6;
      if ((short)uVar6 <= sVar1 >> 1) {
        uVar3 = (int)sVar1 >> 1;
      }
      uVar2 = (ushort)uVar3;
      uVar5 = uVar6;
      if ((short)uVar6 <= sVar4 >> 1) {
        uVar5 = (int)sVar4 >> 1;
      }
      local_c = local_c - 1;
    } while (local_c != 0);
  }
  iVar7 = ((int)param_1 + (int)(short)uVar2 * (int)param_2 + iVar7) *
          (int)(char)(&DAT_006571f4)[*(short *)(unaff_EDI + 0xc)];
  return ((int)(iVar7 + (iVar7 >> 0x1f & 7U)) >> 3) + *(int *)(unaff_EDI + 0x2c);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
