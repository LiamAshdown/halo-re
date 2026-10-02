// rasterizer_resolve_and_cache_submap_c  (Ghidra: FUN_00518a60, unnamed)
// address 0x518a60, size 217 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: the texture-cache aware variant of chimera__rasterizer_set_texture: after resolving
//   the tag's frame it asks texture_cache_get(bitmap, 0, 1); a cache miss returns 1 at once
//   (nothing bound). A resolved bitmap of the wrong type, or no usable tag, falls back to the
//   GlobalsRasterizerData default. The bind goes through rasterizer_bind_texture_d3d9 and the
//   size is cached in rasterizer_bound_bitmap_size_c (0x006d9874); every path that binds, and
//   every failure, returns 0 (the success latch in BL is never raised in this body).
// Spot-check fix (phase 4 review, texture bind helpers 0x518680..0x518a60 rewritten together
//   from the raw code): the earlier rewrite passed NULL where the binary forwards the
//   resolved BitmapData in ESI, called bitmap_group_get_bitmap_data and texture_cache_get
//   without their register arguments (EAX tag / DX frame index, EAX bitmap) and did not pass the frame and swapped the stage and type roles.
// register convention: EAX = bitmap tag id, DI = bitmap type, stack = (stage, default_index, frame);
//   returns a bool in AL ("still loading").

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances;                                 // 0x0087bc14
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern uint8_t console_debug_toggle_689409;                         // 0x00689409
// blam-cc: EAX -> bitmap_tag_id, DX -> index
extern BitmapData *bitmap_group_get_bitmap_data(uint32_t bitmap_tag_id, int16_t index); // 0x43f250
// blam-cc: EAX -> bitmap
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x00444550
// blam-cc: ESI -> bitmap, stack -> stage
extern uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap); // 0x518680

extern int16_t rasterizer_bound_bitmap_size_c[2];                    // 0x006d9874

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

// blam-cc: EAX -> bitmap_tag_id, DI -> bitmap_type, stack -> (stage, default_index, frame)
uint8_t rasterizer_resolve_and_cache_submap_c(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage,
                                              int16_t default_index, int16_t frame)
{
    uint8_t resolved;
    BitmapData *data = rasterizer_tag_bitmap(bitmap_tag_id, bitmap_type, default_index, frame, &resolved);

    if (resolved) {
        if (texture_cache_get(data, 0, 1) == 0) {
            return 1;
        }
    }
    if (!resolved || *(int16_t *)&((struct BitmapData *)data)->type != bitmap_type) {
        data = rasterizer_default_bitmap(bitmap_type, default_index);
        if (data == 0) {
            return 0;
        }
    }
    rasterizer_bind_texture_d3d9(stage, data);
    rasterizer_bound_bitmap_size_c[0] = (int16_t)data->width;
    rasterizer_bound_bitmap_size_c[1] = (int16_t)data->height;
    return 0;
}

#if 0
Original Ghidra decompilation (0x518a60):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_00518a60(undefined4 param_1,short param_2)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  undefined2 extraout_var;
  uint uVar3;
  short unaff_DI;

  if ((((DAT_00689409 == '\0') && (param_2 == 3)) || (in_EAX == 0xffffffff)) ||
     (*(int *)(*(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x60) < 1)) {
LAB_00518ac2:
    uVar3 = *(uint *)(unaff_DI * 0x10 + 0xb8 + DAT_0071d164);
    if (((uVar3 == 0xffffffff) ||
        (uVar3 = *(uint *)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), uVar3 == 0)) ||
       ((param_2 < 0 ||
        ((*(int *)(uVar3 + 0x60) <= (int)param_2 ||
         (iVar1 = param_2 * 0x30 + *(int *)(uVar3 + 100), iVar1 == 0)))))) goto LAB_00518b2d;
  }
  else {
    iVar1 = bitmap_group_get_bitmap_data();
    iVar2 = texture_cache_get(0,1);
    if (iVar2 == 0) {
      return 1;
    }
    if (*(short *)(iVar1 + 10) != unaff_DI) goto LAB_00518ac2;
  }
  FUN_00518680(param_1);
  _DAT_006d9874 = *(undefined2 *)(iVar1 + 4);
  uVar3 = CONCAT22(extraout_var,_DAT_006d9874);
  _DAT_006d9876 = *(undefined2 *)(iVar1 + 6);
LAB_00518b2d:
  return uVar3 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
