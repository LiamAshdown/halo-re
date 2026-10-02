// bitmap_compute_texture_data_size  (Ghidra: bitmap_compute_texture_data_size, already named)
// address 0x5146c0, size 343 bytes
// name confidence: 0.55  rewrite confidence: 0.7
// evidence: sums bitmap_data_calculate_mip_level_pixel_count(...)*bits_per_pixel/8 over every
//   mip level from bitmap_compute_mipmap_count, adding a second (quartered, block-rounded) term
//   for compressed formats (BitmapData.flags bit 4, matching the "compressed" bit this module's
//   other bitmap helpers use) and dividing the total by 6 for cube maps (type == 2) before
//   multiplying back by 6 at the very end -- net no-op for cube maps except for the intermediate
//   truncation, preserved verbatim below.
// register convention: BitmapData pointer in in_EAX (unresolved register read).
//   // blam-cc: in_EAX -> bitmap
// UNSURE: bitmap_data_calculate_mip_level_pixel_count is called with no visible arguments at
//   this call site; almost certainly (bitmap, mip_index) but not confirmed. bits_per_pixel table
//   size (19) taken from BitmapDataFormat_t's enum range in types/tags.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int8_t bitmap_format_bits_per_pixel[];            // 0x006571f4 indexed by BitmapDataFormat
// blam-cc: ESI -> bitmap, ECX -> mip_level
extern uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t mip_level); // 0x43fc10
extern uint32_t bitmap_compute_mipmap_count(BitmapData *bitmap); // 0x5145a0

// blam-cc: in_EAX -> bitmap
// Computes the total byte size of a bitmap's pixel data across every mip level (see the file
// header for the compressed-format and cube-map adjustments), rounded up to a multiple of
// 0x80 bytes.
int32_t bitmap_compute_texture_data_size(BitmapData *bitmap)
{
    int32_t total = 0;
    int32_t level_bytes = 0;
    int16_t mip_count;
    int16_t level;
    uint8_t shift;
    int8_t bits_per_pixel;
    uint16_t flags;

    mip_count = (int16_t)bitmap_compute_mipmap_count(bitmap);

    if (mip_count >= 0) {
        bits_per_pixel = bitmap_format_bits_per_pixel[bitmap->format];
        flags = bitmap->flags;
        level = 0;
        shift = 0;
        do {
            level_bytes = (int32_t)bitmap_data_calculate_mip_level_pixel_count(bitmap, level); // ESI bitmap, ECX level (0x514705)
            level_bytes = level_bytes * bits_per_pixel;
            level_bytes = (level_bytes + ((level_bytes >> 0x1f) & 7)) >> 3;

            if ((flags & 0x10) != 0) {
                int16_t width_at_level;
                int16_t height_at_level;
                int32_t height_bytes;

                // the original shifts the dimensions as signed 16-bit values (sar dx,cl / sar ax,cl)
                width_at_level = ((int16_t)bitmap->width >> shift) < 2 ? 1 : (int16_t)((int16_t)bitmap->width >> shift);
                if ((bitmap->flags & 2) != 0) {
                    width_at_level = width_at_level + ((uint8_t)(-(int8_t)width_at_level) & 3);
                }
                height_bytes = (int32_t)width_at_level * (int32_t)bits_per_pixel;

                height_at_level = ((int16_t)bitmap->height >> shift) < 2 ? 1 : (int16_t)((int16_t)bitmap->height >> shift);
                if ((bitmap->flags & 2) != 0) {
                    height_at_level = height_at_level + ((uint8_t)(-(int8_t)height_at_level) & 3);
                }
                level_bytes = level_bytes + (int32_t)height_at_level *
                              (-(((height_bytes + ((height_bytes >> 0x1f) & 7)) >> 3)) & 0x3f);
            }

            if (bitmap->type == 2) {
                level_bytes = level_bytes / 6;
            }
            total = total + level_bytes;
            level = level + 1;
            shift = shift + 1;
        } while (level <= mip_count);
    }

    total = total + (-total & 0x7f);
    if (bitmap->type != 2) {
        return total;
    }
    return total * 6;
}

#if 0
Original Ghidra decompilation (0x5146c0):

int bitmap_compute_texture_data_size(void)

{
  char cVar1;
  ushort uVar2;
  short sVar3;
  short sVar4;
  int in_EAX;
  int iVar5;
  byte bVar6;
  short sVar7;
  int iVar8;
  int local_14;

  iVar8 = 0;
  local_14 = 0;
  sVar3 = bitmap_compute_mipmap_count();
  sVar7 = 0;
  if (-1 < sVar3) {
    cVar1 = (&DAT_006571f4)[*(short *)(in_EAX + 0xc)];
    uVar2 = *(ushort *)(in_EAX + 0xe);
    bVar6 = 0;
    do {
      iVar8 = bitmap_data_calculate_mip_level_pixel_count();
      iVar8 = iVar8 * cVar1;
      iVar8 = (int)(iVar8 + (iVar8 >> 0x1f & 7U)) >> 3;
      if ((uVar2 & 0x10) != 0) {
        if (*(short *)(in_EAX + 4) >> ((byte)sVar7 & 0x1f) < 2) {
          sVar4 = 1;
        }
        else {
          sVar4 = *(short *)(in_EAX + 4) >> (bVar6 & 0x1f);
        }
        if ((*(ushort *)(in_EAX + 0xe) & 2) != 0) {
          sVar4 = sVar4 + ((byte)-(char)sVar4 & 3);
        }
        iVar5 = (int)sVar4 * (int)cVar1;
        if (*(short *)(in_EAX + 6) >> ((byte)sVar7 & 0x1f) < 2) {
          sVar4 = 1;
        }
        else {
          sVar4 = *(short *)(in_EAX + 6) >> (bVar6 & 0x1f);
        }
        if ((*(ushort *)(in_EAX + 0xe) & 2) != 0) {
          sVar4 = sVar4 + ((byte)-(char)sVar4 & 3);
        }
        iVar8 = iVar8 + (int)sVar4 * (-((int)(iVar5 + (iVar5 >> 0x1f & 7U)) >> 3) & 0x3fU);
      }
      if (*(short *)(in_EAX + 10) == 2) {
        iVar8 = iVar8 / 6;
      }
      iVar8 = local_14 + iVar8;
      sVar7 = sVar7 + 1;
      bVar6 = bVar6 + 1;
      local_14 = iVar8;
    } while (sVar7 <= sVar3);
  }
  iVar8 = iVar8 + (-iVar8 & 0x7fU);
  if (*(short *)(in_EAX + 10) != 2) {
    return iVar8;
  }
  return iVar8 * 6;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
