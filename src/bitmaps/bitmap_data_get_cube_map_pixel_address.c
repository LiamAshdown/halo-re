// bitmap_data_get_cube_map_pixel_address  (Ghidra: FUN_0043fa90; named here, not yet renamed in
// Ghidra/CEA)
// address 0x43fa90, size 136 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_types_notes.md register-convention table (objdump-derived) plus
//   its own arithmetic: it walks the mip chain exactly like the sibling 2D routine
//   bitmap_data_get_row_address (0x43f8e0, out of this session's range) and the 3D routine
//   FUN_0043f990 (0x43f990, out of this session's range), but accumulates width*width*6 per
//   earlier level (6 faces) and indexes the target level as (face*width + y)*width + x -- the
//   layout of a 6-face cube map mip level in the pixel buffer. bitmap_data_get_pixel_address
//   (0x43fb20, this module) dispatches to it for BitmapDataType_t == bitmapdatatype_cube_map.
// register convention: EDI bitmap, CX (in_CX) mip level, stack x, y, face.
//   // blam-cc: ECX (CX) -> mip_level, EDI -> bitmap, stack -> x, y, face
// Note: face/x/y are not range-checked here (unlike the tag-side bounds bitmap_data_verify
//   enforces on width/height); out-of-range callers would compute a pixel address outside the
//   allocated buffer, exactly as the original does.

#include "tags.h"
#include "bitmaps.h"

extern int8_t bitmap_format_bits_per_pixel[k_bitmap_data_format_count]; // 0x006571f4 (types/bitmaps.h)

// blam-cc: ECX (CX) -> mip_level, EDI -> bitmap, stack -> x, y, face
// Computes the byte address of pixel (x, y) on cube map face `face` at mip level `mip_level`
// within bitmap's pixel buffer, accounting for the 6 faces stored per mip level.
void *bitmap_data_get_cube_map_pixel_address(BitmapData *bitmap, int32_t mip_level, int16_t x, int16_t y, int16_t face)
{
    int16_t width;
    int16_t min_dimension;
    int16_t levels_remaining;
    int32_t earlier_levels_pixel_count;
    int32_t pixel_index;
    int32_t bit_offset;

    width = (int16_t)bitmap->width;
    min_dimension = ((bitmap->flags & _bitmap_data_compressed_bit) != 0) ?
        k_bitmap_compressed_block_dimension : 1;

    earlier_levels_pixel_count = 0;
    levels_remaining = (int16_t)mip_level;
    while (levels_remaining > 0) {
        earlier_levels_pixel_count += (int32_t)width * width * k_cube_map_face_count;
        width = ((width >> 1) < min_dimension) ? min_dimension : (int16_t)(width >> 1); // sar ax,1
        levels_remaining--;
    }

    // x, y and face are read as words (movsx at 0x43fae4..0x43faf9).
    pixel_index = (int32_t)x + ((int32_t)face * (int32_t)width + (int32_t)y) * (int32_t)width +
        earlier_levels_pixel_count;
    bit_offset = pixel_index * (int32_t)bitmap_format_bits_per_pixel[bitmap->format];

    return *(uint8_t **)&((struct BitmapData *)bitmap)->pixel_base + bit_offset / 8;
}

#if 0
Original Ghidra decompilation (0x43fa90):

int FUN_0043fa90(short param_1,short param_2,short param_3)

{
  short sVar1;
  ushort uVar2;
  uint uVar3;
  ushort in_CX;
  int iVar4;
  uint uVar5;
  int iVar6;
  int unaff_EDI;
  uint local_4;

  uVar2 = *(ushort *)(unaff_EDI + 4);
  uVar3 = (uint)uVar2;
  iVar6 = 0;
  uVar5 = (-(uint)((*(byte *)(unaff_EDI + 0xe) & 2) != 0) & 3) + 1;
  if (0 < (short)in_CX) {
    local_4 = (uint)in_CX;
    do {
      sVar1 = (short)uVar3;
      iVar4 = (int)sVar1;
      iVar6 = iVar6 + iVar4 * iVar4 * 6;
      uVar3 = uVar5;
      if ((short)uVar5 <= sVar1 >> 1) {
        uVar3 = iVar4 >> 1;
      }
      uVar2 = (ushort)uVar3;
      local_4 = local_4 - 1;
    } while (local_4 != 0);
  }
  iVar6 = ((int)param_1 +
          ((int)param_3 * (int)(short)uVar2 + (int)param_2) * (int)(short)uVar2 + iVar6) *
          (int)(char)(&DAT_006571f4)[*(short *)(unaff_EDI + 0xc)];
  return ((int)(iVar6 + (iVar6 >> 0x1f & 7U)) >> 3) + *(int *)(unaff_EDI + 0x2c);
}
#endif
