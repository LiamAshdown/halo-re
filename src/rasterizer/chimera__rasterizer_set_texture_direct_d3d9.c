// chimera__rasterizer_set_texture_direct_d3d9  (Ghidra: FUN_00518770; Chimera name, hint only)
// address 0x518770, size 100 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: identical to chimera__rasterizer_set_texture_direct_d3dx 0x518700 but binds through
//   rasterizer_bind_texture_d3d9 0x518680 (its result is ignored; 1 is returned once a bitmap
//   was found).
// Spot-check fix (phase 4 review, texture bind helpers 0x518680..0x518a60 rewritten together
//   from the raw code): the earlier rewrite passed NULL where the binary forwards the
//   resolved BitmapData in ESI, called bitmap_group_get_bitmap_data and texture_cache_get
//   without their register arguments (EAX tag / DX frame index, EAX bitmap) and bound a NULL bitmap.
// register convention: EAX = bitmap tag id, stack = (stage, frame); returns a bool in AL.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances;                                 // 0x0087bc14
// blam-cc: ESI -> bitmap, stack -> stage
extern uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap); // 0x518680

// Picks frame (mod the bitmap count) out of the bitmap group and binds it; 0 when the tag is
// NONE, the group is empty or the entry is missing.
static BitmapData *bitmap_group_frame(uint32_t bitmap_tag_id, int16_t frame)
{
    Bitmap *bitmap = (Bitmap *)tag_instances[bitmap_tag_id & 0xffff].data;
    int32_t count = (int32_t)bitmap->bitmap_data.count;
    int16_t index;

    if (count <= 0 || bitmap == 0) {
        return 0;
    }
    index = (int16_t)((int32_t)frame % count);
    if (index < 0 || index >= count) {
        return 0;
    }
    return (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + index * 0x30);
}

// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, frame)
uint8_t chimera__rasterizer_set_texture_direct_d3d9(uint32_t bitmap_tag_id, int16_t stage, int16_t frame)
{
    BitmapData *data;

    if (bitmap_tag_id == 0xffffffff) {
        return 0;
    }
    data = bitmap_group_frame(bitmap_tag_id, frame);
    if (data == 0) {
        return 0;
    }
    rasterizer_bind_texture_d3d9(stage, data);
    return 1;
}

#if 0
Original Ghidra decompilation (0x518770):

undefined4 chimera__rasterizer_set_texture_direct_d3d9(undefined4 param_1,short param_2)

{
  int iVar1;
  int iVar2;
  uint in_EAX;
  short sVar3;

  if (in_EAX == 0xffffffff) {
    return 0;
  }
  iVar1 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar2 = *(int *)(iVar1 + 0x60);
  if (0 < iVar2) {
    if ((((iVar1 != 0) && (sVar3 = (short)((int)param_2 % iVar2), -1 < sVar3)) && (sVar3 < iVar2))
       && (sVar3 * 0x30 + *(int *)(iVar1 + 100) != 0)) {
      FUN_00518680(param_1);
      return 1;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
