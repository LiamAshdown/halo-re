// bitmap_data_get_volume_pixel_address  (Ghidra: FUN_0043f990, renamed)
// address 0x43f990, size 241 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_functions.md ("Computes the byte address of a pixel within a 3D
// (volume) bitmap_data's pixel buffer given x, y, z coordinates."); types/bitmaps.h
// bitmap_format_bits_per_pixel, bitmap_data_flags (_bitmap_data_compressed_bit),
// k_bitmap_compressed_block_dimension. Sibling of bitmap_data_get_row_address (0x43f8e0, same
// batch): same inlined per-mip-level walk, extended with a depth dimension that floors to 1
// every halving regardless of the compressed flag (matching
// out/phase4/bitmaps_functions.md's summary of bitmap_data_calculate_mip_depth, 0x43fbe0,
// outside this batch). The depth floor test `(depth & 0xfffe) < 2` is transcribed below as the
// equivalent `depth < 2`: for every non-negative int16_t depth value the two conditions agree
// (clearing bit 0 only changes odd values, and both 0 and 1 mask to 0, both >= 2 mask to >= 2),
// so this is not a behavioural change.
// register convention: ECX = BitmapData *bitmap_data, stack -> int16_t x, int16_t y, int16_t z,
// int16_t mip_level.
//   // blam-cc: ECX -> bitmap_data, stack -> x, y, z, mip_level

#include "tags.h"
#include "memory.h"
#include "bitmaps.h"

extern int8_t bitmap_format_bits_per_pixel[k_bitmap_data_format_count]; // 0x006571f4 (types/bitmaps.h)

// blam-cc: ECX -> bitmap_data, stack -> x, y, z, mip_level
void *bitmap_data_get_volume_pixel_address(BitmapData *bitmap_data, int16_t x, int16_t y,
    int16_t z, int16_t mip_level)
{
    int16_t width = (int16_t)bitmap_data->width;
    int16_t height = (int16_t)bitmap_data->height;
    int16_t depth = (int16_t)bitmap_data->depth;
    int16_t min_dimension = (bitmap_data->flags & _bitmap_data_compressed_bit) ?
        k_bitmap_compressed_block_dimension : 1;
    int32_t voxel_offset = 0;
    int32_t bit_offset;
    int16_t level; // the original tests mip_level as a signed word (test bp,bp; jle 0x43fa42)
    uint32_t base;

    for (level = mip_level; level > 0; level--) {
        voxel_offset += (int32_t)width * (int32_t)height * (int32_t)depth;
        width  = (width  >> 1 >= min_dimension) ? (int16_t)(width  >> 1) : min_dimension;
        height = (height >> 1 >= min_dimension) ? (int16_t)(height >> 1) : min_dimension;
        depth  = (depth < 2) ? 1 : (int16_t)(depth >> 1); // depth always floors to 1, no compressed clamp
    }

    voxel_offset += (int32_t)x + ((int32_t)height * (int32_t)z + (int32_t)y) * (int32_t)width;
    bit_offset = voxel_offset * (int32_t)bitmap_format_bits_per_pixel[bitmap_data->format];

    base = *(uint32_t *)bitmap_data->_pad_2c;
    return (void *)(base + (uint32_t)((bit_offset + ((bit_offset >> 31) & 7)) >> 3));
}

#if 0
Original Ghidra decompilation (0x43f990):

int FUN_0043f990(short param_1,short param_2,short param_3,ushort param_4)

{
  short sVar1;
  ushort uVar2;
  uint uVar3;
  int iVar4;
  int in_ECX;
  uint uVar5;
  short sVar6;
  ushort uVar7;
  uint uVar8;
  ushort uVar9;
  uint uVar10;
  uint local_10;

  uVar9 = param_4;
  uVar2 = *(ushort *)(in_ECX + 4);
  uVar3 = (uint)uVar2;
  uVar7 = *(ushort *)(in_ECX + 6);
  uVar8 = (uint)uVar7;
  uVar10 = (uint)*(ushort *)(in_ECX + 8);
  uVar5 = (-(uint)((*(byte *)(in_ECX + 0xe) & 2) != 0) & 3) + 1;
  _param_4 = 0;
  if (0 < (short)uVar9) {
    local_10 = (uint)uVar9;
    do {
      uVar9 = (ushort)uVar10;
      sVar1 = (short)uVar3;
      sVar6 = (short)uVar8;
      _param_4 = _param_4 + (int)sVar1 * (int)sVar6 * (int)(short)uVar9;
      uVar3 = uVar5;
      if ((short)uVar5 <= sVar1 >> 1) {
        uVar3 = (int)sVar1 >> 1;
      }
      uVar2 = (ushort)uVar3;
      uVar8 = uVar5;
      if ((short)uVar5 <= sVar6 >> 1) {
        uVar8 = (int)sVar6 >> 1;
      }
      uVar7 = (ushort)uVar8;
      if ((short)(uVar9 & 0xfffe) < 2) {
        uVar10 = 1;
      }
      else {
        uVar10 = (int)(short)uVar9 >> 1;
      }
      local_10 = local_10 - 1;
    } while (local_10 != 0);
  }
  iVar4 = ((int)param_1 +
          ((int)(short)uVar7 * (int)param_3 + (int)param_2) * (int)(short)uVar2 + _param_4) *
          (int)(char)(&DAT_006571f4)[*(short *)(in_ECX + 0xc)];
  return ((int)(iVar4 + (iVar4 >> 0x1f & 7U)) >> 3) + *(int *)(in_ECX + 0x2c);
}
#endif
