// rasterizer_validate_and_rebind_texture  (Ghidra: FUN_005187e0, unnamed)
// address 0x5187e0, size 118 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: the same frame lookup as 0x518770, then texture_cache_get(bitmap, 0, 1): when the
//   cache has no texture for it yet the function returns 1 without binding; otherwise it binds
//   through rasterizer_bind_texture_d3d9 and returns 0. It also returns 0 for a missing bitmap.
// Spot-check fix (phase 4 review, texture bind helpers 0x518680..0x518a60 rewritten together
//   from the raw code): the earlier rewrite passed NULL where the binary forwards the
//   resolved BitmapData in ESI, called bitmap_group_get_bitmap_data and texture_cache_get
//   without their register arguments (EAX tag / DX frame index, EAX bitmap) and bound a NULL bitmap.
// register convention: EAX = bitmap tag id, stack = (stage, frame); returns a bool in AL
//   ("still loading").

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"

extern tag_instance *tag_instances;                                 // 0x0087bc14
// blam-cc: EAX -> bitmap
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x00444550
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
uint8_t rasterizer_validate_and_rebind_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t frame)
{
    BitmapData *data;

    if (bitmap_tag_id == 0xffffffff) {
        return 0;
    }
    data = bitmap_group_frame(bitmap_tag_id, frame);
    if (data == 0) {
        return 0;
    }
    if (texture_cache_get(data, 0, 1) == 0) {
        return 1;
    }
    rasterizer_bind_texture_d3d9(stage, data);
    return 0;
}

#if 0
Original Ghidra decompilation (0x5187e0):

undefined4 FUN_005187e0(undefined4 param_1,short param_2)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  short sVar3;

  if (in_EAX == 0xffffffff) {
    return 0;
  }
  iVar2 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar1 = *(int *)(iVar2 + 0x60);
  if (0 < iVar1) {
    if ((((iVar2 != 0) && (sVar3 = (short)((int)param_2 % iVar1), -1 < sVar3)) && (sVar3 < iVar1))
       && (sVar3 * 0x30 + *(int *)(iVar2 + 100) != 0)) {
      iVar2 = texture_cache_get(0,1);
      if (iVar2 == 0) {
        return 1;
      }
      FUN_00518680(param_1);
    }
  }
  return 0;
}
#endif
