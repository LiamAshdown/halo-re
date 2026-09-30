// chimera__rasterizer_set_texture  (Ghidra: chimera__rasterizer_set_texture; Chimera name, hint only)
// address 0x518960, size 245 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: the d3d9 twin of rasterizer_resolve_and_cache_submap_b 0x518860: same resolution
//   (tag frame with a matching type, else the GlobalsRasterizerData default of that type, entry
//   default_index), binds through rasterizer_bind_texture_d3d9 and caches width/height into
//   rasterizer_bound_bitmap_size_a (0x006d986c). Returns that global's address, or NULL.
// Spot-check fix (phase 4 review, texture bind helpers 0x518680..0x518a60 rewritten together
//   from the raw code): the earlier rewrite passed NULL where the binary forwards the
//   resolved BitmapData in ESI, called bitmap_group_get_bitmap_data and texture_cache_get
//   without their register arguments (EAX tag / DX frame index, EAX bitmap) and had only three stack parameters: the fourth (frame, 0x51899e) was missing and the stage passed to the bind was the default index.
// register convention: EAX = bitmap tag id, stack = (stage, bitmap_type, default_index, frame).

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
// blam-cc: ESI -> bitmap, stack -> stage


extern int16_t rasterizer_bound_bitmap_size_a[2];                    // 0x006d986c

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

// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)
int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
                                         int16_t default_index, int16_t frame)
{
    uint8_t resolved;
    BitmapData *data = rasterizer_tag_bitmap(bitmap_tag_id, bitmap_type, default_index, frame, &resolved);

    if (!resolved || *(int16_t *)&((struct BitmapData *)data)->type != bitmap_type) {
        data = rasterizer_default_bitmap(bitmap_type, default_index);
        if (data == 0) {
            return 0;
        }
    }
    rasterizer_bind_texture_d3d9(stage, data);
    rasterizer_bound_bitmap_size_a[0] = (int16_t)data->width;
    rasterizer_bound_bitmap_size_a[1] = (int16_t)data->height;
    return rasterizer_bound_bitmap_size_a;
}

#if 0
Original Ghidra decompilation (0x518960):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint chimera__rasterizer_set_texture(undefined4 param_1,short param_2,short param_3)

{
  uint uVar1;
  byte bVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;

  iVar4 = DAT_0087bc14;
  bVar2 = 0;
  if ((((DAT_00689409 == '\0') && (param_3 == 3)) || (in_EAX == 0xffffffff)) ||
     (*(int *)(*(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x60) < 1)) {
LAB_005189d6:
    uVar1 = *(uint *)(param_2 * 0x10 + 0xb8 + DAT_0071d164);
    if (((uVar1 == 0xffffffff) ||
        (iVar4 = *(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + iVar4), iVar4 == 0)) ||
       ((param_3 < 0 ||
        ((*(int *)(iVar4 + 0x60) <= (int)param_3 ||
         (iVar4 = param_3 * 0x30 + *(int *)(iVar4 + 100), iVar4 == 0)))))) goto LAB_00518a44;
    FUN_00518680(param_1);
    _DAT_006d986c = *(undefined2 *)(iVar4 + 4);
    _DAT_006d986e = *(undefined2 *)(iVar4 + 6);
  }
  else {
    iVar3 = bitmap_group_get_bitmap_data();
    if (*(short *)(iVar3 + 10) != param_2) goto LAB_005189d6;
    FUN_00518680(param_1);
    _DAT_006d986c = *(undefined2 *)(iVar3 + 4);
    _DAT_006d986e = *(undefined2 *)(iVar3 + 6);
  }
  bVar2 = 1;
LAB_00518a44:
  return -(uint)bVar2 & 0x6d986c;
}
#endif
