// bitmap_compute_mipmap_count  (Ghidra: bitmap_compute_mipmap_count, already named)
// address 0x5145a0, size 287 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: unaff_EBX's field offsets (+4/+6/+8 width/height/depth, +0xe flags, +0x14
//   mipmap_count) match BitmapData exactly (types/tags.h); flags bit 0 gates on
//   power_of_two_dimensions, bit 4 (linear) disables mipmapping outright, and bit 1 selects a
//   different (4-block-rounded) size reduction consistent with a compressed format.
// register convention: BitmapData pointer in unaff_EBX (unresolved register read).
//   // blam-cc: unaff_EBX -> bitmap
// FIXED (objdump 0x5145a0): both tail calls to uint32_log2_floor pass ECX = the largest dimension; was: this
//   is a straight, line by line translation of the original arithmetic (renamed to struct field
//   accesses where the offset is one of BitmapData's) rather than a re-derivation of intended
//   behaviour, since several of the intermediate values (sVar5/sVar7/uVar3 reuse) are hard to
//   name meaningfully without more evidence.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern int32_t uint32_log2_floor(uint32_t value); // 0x4cb740; ECX -> value

// blam-cc: unaff_EBX -> bitmap
// Computes the number of mipmap levels to generate for a bitmap, honouring its requested
// mipmap_count when the bitmap's dimensions can naturally support at least that many levels.
// the result is a 16-bit level count: on the mipmap_count path the original loads only AX, leaving the upper
// half of EAX from earlier code; its caller reads AX
int16_t bitmap_compute_mipmap_count(BitmapData *bitmap)
{
    uint16_t flags = bitmap->flags;
    uint32_t result = 0;

    if ((flags & 1) == 0 || (flags & 0x10) != 0) {
        return result;
    }

    if ((flags & 2) == 0) {
        int16_t height = (int16_t)bitmap->height;
        int16_t depth = (int16_t)bitmap->depth;
        int16_t max_hd = (height <= depth) ? depth : height;
        int16_t width = (int16_t)bitmap->width;
        int32_t max_dim;
        int16_t levels;

        if (width <= max_hd) {
            max_dim = (height <= depth) ? depth : height;
        } else {
            max_dim = width;
        }

        levels = 0;
        if (max_dim != 0) {
            uint32_t v = (uint32_t)max_dim;
            while (v != 1) {
                v = v >> 1;
                levels = levels + 1;
            }
        }

        result = bitmap->mipmap_count;
        if (levels < (int16_t)bitmap->mipmap_count) {
            int16_t hd = (height <= depth) ? depth : height;
            if (hd < width) {
                return (uint32_t)uint32_log2_floor((uint32_t)max_dim); // 0x514631: ECX = the largest dimension
            }
            return (uint32_t)uint32_log2_floor((uint32_t)max_dim); // 0x51463b
        }
    } else {
        int16_t depth = (int16_t)bitmap->depth;
        int32_t width_blocks = ((int32_t)(int16_t)bitmap->width +
                                 (((int32_t)(int16_t)bitmap->width >> 0x1f) & 3)) >> 2;
        int32_t max1 = (width_blocks <= depth) ? depth : width_blocks;
        int32_t height_rounded = (int32_t)(int16_t)bitmap->height +
                                  (((int32_t)(int16_t)bitmap->height >> 0x1f) & 3);
        int32_t height_blocks = height_rounded >> 2;
        int32_t max2;
        int16_t levels;

        if (height_blocks <= max1) {
            max2 = (width_blocks <= depth) ? depth : width_blocks;
        } else {
            max2 = height_blocks;
        }

        levels = 0;
        if (max2 != 0) {
            uint32_t v = (uint32_t)max2;
            while (v != 1) {
                v = v >> 1;
                levels = levels + 1;
            }
        }

        result = bitmap->mipmap_count;
        if (levels < (int16_t)bitmap->mipmap_count) {
            int32_t wb = width_blocks;
            if (wb <= depth) {
                wb = depth;
            }
            if (wb < height_blocks) {
                return (uint32_t)uint32_log2_floor((uint32_t)max2); // 0x5146a0: ECX = the largest block dimension
            }
            return (uint32_t)uint32_log2_floor((uint32_t)max2); // 0x5146a9
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x5145a0):

uint bitmap_compute_mipmap_count(void)

{
  ushort uVar1;
  short sVar2;
  uint uVar3;
  int iVar4;
  short sVar5;
  uint uVar6;
  short sVar7;
  int unaff_EBX;
  uint uVar8;
  uint uVar9;
  uint uVar10;

  uVar1 = *(ushort *)(unaff_EBX + 0xe);
  uVar3 = 0;
  if (((uVar1 & 1) != 0) && ((uVar1 & 0x10) == 0)) {
    if ((uVar1 & 2) == 0) {
      sVar7 = *(short *)(unaff_EBX + 6);
      sVar2 = *(short *)(unaff_EBX + 8);
      sVar5 = sVar7;
      if (sVar7 <= sVar2) {
        sVar5 = sVar2;
      }
      uVar9 = (uint)*(short *)(unaff_EBX + 4);
      uVar3 = uVar9;
      if (((int)uVar9 <= (int)sVar5) && (uVar3 = (int)sVar7, sVar7 <= sVar2)) {
        uVar3 = (int)sVar2;
      }
      sVar5 = 0;
      if (uVar3 != 0) {
        for (; uVar3 != 1; uVar3 = uVar3 >> 1) {
          sVar5 = sVar5 + 1;
        }
      }
      uVar3 = (uint)*(ushort *)(unaff_EBX + 0x14);
      if (sVar5 < (short)*(ushort *)(unaff_EBX + 0x14)) {
        if (sVar7 <= sVar2) {
          sVar7 = sVar2;
        }
        if ((int)sVar7 < (int)uVar9) {
          uVar3 = uint32_log2_floor();
          return uVar3;
        }
        uVar3 = uint32_log2_floor();
        return uVar3;
      }
    }
    else {
      uVar10 = (uint)*(short *)(unaff_EBX + 8);
      uVar9 = (int)((int)*(short *)(unaff_EBX + 6) + ((int)*(short *)(unaff_EBX + 6) >> 0x1f & 3U))
              >> 2;
      uVar3 = uVar9;
      if ((int)uVar9 <= (int)uVar10) {
        uVar3 = uVar10;
      }
      iVar4 = (int)*(short *)(unaff_EBX + 4) + ((int)*(short *)(unaff_EBX + 4) >> 0x1f & 3U);
      uVar8 = iVar4 >> 2;
      uVar6 = uVar8;
      if (((int)uVar8 <= (int)uVar3) && (uVar6 = uVar9, (int)uVar9 <= (int)uVar10)) {
        uVar6 = uVar10;
      }
      sVar7 = 0;
      if (uVar6 != 0) {
        for (; uVar6 != 1; uVar6 = uVar6 >> 1) {
          sVar7 = sVar7 + 1;
        }
      }
      uVar3 = CONCAT22((short)((uint)iVar4 >> 0x10),*(short *)(unaff_EBX + 0x14));
      if (sVar7 < *(short *)(unaff_EBX + 0x14)) {
        if ((int)uVar9 <= (int)uVar10) {
          uVar9 = uVar10;
        }
        if ((int)uVar9 < (int)uVar8) {
          uVar3 = uint32_log2_floor();
          return uVar3;
        }
        uVar3 = uint32_log2_floor();
        return uVar3;
      }
    }
  }
  return uVar3;
}
#endif
