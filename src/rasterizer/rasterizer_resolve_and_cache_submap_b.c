// rasterizer_resolve_and_cache_submap_b  (Ghidra: FUN_00518860, unnamed)
// address 0x518860, size 251 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: resolves frame (mod count) of the tag's bitmap group and requires its type
//   (BitmapData +0xa) to equal bitmap_type; otherwise (or for a NONE/empty tag, or stage class 3
//   while 0x00689409 is clear) falls back to the GlobalsRasterizerData default bitmap of that
//   type, entry default_index. Binds through rasterizer_bind_texture_d3dx with the caller's
//   effect slot (moved into EDI from the fourth stack argument) and caches width/height into
//   rasterizer_bound_bitmap_size_b (0x006d9870). Returns that global's address, or NULL.
// Spot-check fix (phase 4 review, texture bind helpers 0x518680..0x518a60 rewritten together
//   from the raw code): the earlier rewrite passed NULL where the binary forwards the
//   resolved BitmapData in ESI, called bitmap_group_get_bitmap_data and texture_cache_get
//   without their register arguments (EAX tag / DX frame index, EAX bitmap) and used the wrong stack slots for stage/default index and dropped the frame and effect slot arguments.
// register convention: EAX = bitmap tag id, CX = bitmap type, stack = (stage, default_index,
//   frame, effect_slot).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern tag_instance *tag_instances;                                 // 0x0087bc14
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern uint8_t console_debug_toggle_689409;                         // 0x00689409
// blam-cc: EAX -> bitmap_tag_id, DX -> index
extern BitmapData *bitmap_group_get_bitmap_data(uint32_t bitmap_tag_id, int16_t index); // 0x43f250
// blam-cc: ESI -> bitmap, EDI -> effect_slot, stack -> stage


extern int16_t rasterizer_bound_bitmap_size_b[2];                    // 0x006d9870

// GlobalsRasterizerData default_2d/default_3d/default_cube_map/... (TagDependency array at +0xac,
// tag id at +0xb8) indexed by bitmap type, entry default_index of that bitmap group.
static BitmapData *rasterizer_default_bitmap(int16_t bitmap_type, int16_t default_index)
{
    uint32_t tag = *(uint32_t *)((uint8_t *)rasterizer_globals_data + bitmap_type * 0x10 + 0xb8);
    Bitmap *bitmap;

    if (tag == 0xffffffff) {
        return 0;
    }
    bitmap = (Bitmap *)tag_instances[tag & 0xffff].data;
    if (bitmap == 0 || default_index < 0 || default_index >= (int32_t)bitmap->bitmap_data.count) {
        return 0;
    }
    return (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + default_index * 0x30);
}

// The tag's own frame when the tag is usable and its type matches, else NULL. Stage class 3
// (lightmaps) is refused unless console_debug_toggle_689409 is set.
static BitmapData *rasterizer_tag_bitmap(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t default_index, int16_t frame,
                                         uint8_t *resolved)
{
    Bitmap *bitmap;
    int32_t count;

    *resolved = 0;
    if ((console_debug_toggle_689409 == 0 && default_index == 3) || bitmap_tag_id == 0xffffffff) {
        return 0;
    }
    bitmap = (Bitmap *)tag_instances[bitmap_tag_id & 0xffff].data;
    count = (int32_t)bitmap->bitmap_data.count;
    if (count <= 0) {
        return 0;
    }
    *resolved = 1;
    return bitmap_group_get_bitmap_data(bitmap_tag_id, (int16_t)((int32_t)frame % count));
}

// blam-cc: EAX -> bitmap_tag_id, CX -> bitmap_type, stack -> (stage, default_index, frame, effect_slot)
int16_t *rasterizer_resolve_and_cache_submap_b(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage,
                                               int16_t default_index, int16_t frame, rasterizer_effect_slot *effect_slot)
{
    uint8_t resolved;
    BitmapData *data = rasterizer_tag_bitmap(bitmap_tag_id, bitmap_type, default_index, frame, &resolved);

    if (!resolved || *(int16_t *)&((struct BitmapData *)data)->type != bitmap_type) {
        data = rasterizer_default_bitmap(bitmap_type, default_index);
        if (data == 0) {
            return 0;
        }
    }
    rasterizer_bind_texture_d3dx(stage, data, effect_slot);
    rasterizer_bound_bitmap_size_b[0] = (int16_t)data->width;
    rasterizer_bound_bitmap_size_b[1] = (int16_t)data->height;
    return rasterizer_bound_bitmap_size_b;
}

#if 0
Original Ghidra decompilation (0x518860):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_00518860(undefined4 param_1,short param_2)

{
  uint uVar1;
  byte bVar2;
  uint in_EAX;
  int iVar3;
  short in_CX;
  int iVar4;

  iVar4 = DAT_0087bc14;
  bVar2 = 0;
  if ((((DAT_00689409 == '\0') && (param_2 == 3)) || (in_EAX == 0xffffffff)) ||
     (*(int *)(*(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x60) < 1)) {
LAB_005188d8:
    uVar1 = *(uint *)(in_CX * 0x10 + 0xb8 + DAT_0071d164);
    if (((uVar1 == 0xffffffff) ||
        (iVar4 = *(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + iVar4), iVar4 == 0)) ||
       ((param_2 < 0 ||
        ((*(int *)(iVar4 + 0x60) <= (int)param_2 ||
         (iVar4 = param_2 * 0x30 + *(int *)(iVar4 + 100), iVar4 == 0)))))) goto LAB_0051894a;
    FUN_005186c0(param_1);
    _DAT_006d9870 = *(undefined2 *)(iVar4 + 4);
    _DAT_006d9872 = *(undefined2 *)(iVar4 + 6);
  }
  else {
    iVar3 = bitmap_group_get_bitmap_data();
    if (*(short *)(iVar3 + 10) != in_CX) goto LAB_005188d8;
    FUN_005186c0(param_1);
    _DAT_006d9870 = *(undefined2 *)(iVar3 + 4);
    _DAT_006d9872 = *(undefined2 *)(iVar3 + 6);
  }
  bVar2 = 1;
LAB_0051894a:
  return -(uint)bVar2 & 0x6d9870;
}
#endif
