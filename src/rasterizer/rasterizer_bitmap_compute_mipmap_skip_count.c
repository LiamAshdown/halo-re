// rasterizer_bitmap_compute_mipmap_skip_count  (Ghidra: already named)
// address 0x523f10, size 133 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase2/results/rasterizer_01.json ("Computes how many top mip levels to drop
// from a bitmap based on the texture-quality setting, halving the reported width/height
// accordingly."). `in_EAX` matches types/tags.h BitmapData exactly (width at +4, height at +6,
// mipmap_count at +0x14).
// register convention: bitmap in EAX, out_width in EBX (both live-in), out_height as the
// recognized stack parameter.
// blam-cc: EAX -> bitmap, EBX -> out_width, stack -> out_height

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int8_t renderer_texture_quality; // 0x0068944e, UNSURE owner; clamped to [0,2] skip levels

// Returns how many mip levels were skipped (0 if the quality setting, bitmap flags or mip count
// don't allow skipping), halving *out_width/*out_height once per level skipped.
int32_t rasterizer_bitmap_compute_mipmap_skip_count(BitmapData *bitmap, int16_t *out_width,
                                                      int16_t *out_height)
{
    int32_t max_skip;
    int32_t mipmap_count;
    int32_t remaining;

    if (renderer_texture_quality < 0) {
        max_skip = 0;
    } else if (renderer_texture_quality < 3) {
        max_skip = renderer_texture_quality;
    } else {
        max_skip = 2;
    }

    mipmap_count = bitmap->mipmap_count;
    *out_width = bitmap->width;
    *out_height = bitmap->height;

    if (max_skip != 0 && 0x3f < *out_width && 0x3f < *out_height && 1 < mipmap_count) {
        remaining = max_skip;
        for (; 0 < max_skip; max_skip--) {
            if (max_skip < mipmap_count) {
                *out_width = *out_width / 2;
                *out_height = *out_height / 2;
                mipmap_count = mipmap_count - 1;
            } else {
                remaining = remaining - 1;
            }
        }
        return remaining;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x523f10):

int rasterizer_bitmap_compute_mipmap_skip_count(short *param_1)

{
  short sVar1;
  int in_EAX;
  int iVar2;
  int iVar3;
  short *unaff_EBX;
  int iVar4;

  if (DAT_0068944e < 0) {
    iVar2 = 0;
  }
  else if (DAT_0068944e < 3) {
    iVar2 = (int)DAT_0068944e;
  }
  else {
    iVar2 = 2;
  }
  iVar4 = (int)*(short *)(in_EAX + 0x14);
  *unaff_EBX = *(short *)(in_EAX + 4);
  sVar1 = *(short *)(in_EAX + 6);
  *param_1 = sVar1;
  if ((((iVar2 != 0) && (0x3f < *unaff_EBX)) && (0x3f < sVar1)) && (iVar3 = iVar2, 1 < iVar4)) {
    for (; 0 < iVar2; iVar2 = iVar2 + -1) {
      if (iVar2 < iVar4) {
        *unaff_EBX = *unaff_EBX / 2;
        *param_1 = *param_1 / 2;
        iVar4 = iVar4 + -1;
      }
      else {
        iVar3 = iVar3 + -1;
      }
    }
    return iVar3;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
