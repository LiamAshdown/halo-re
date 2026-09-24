// rasterizer_bind_texture_d3dx  (Ghidra: FUN_005186c0, unnamed)
// address 0x5186c0, size 54 bytes
// name confidence: 0.5   rewrite confidence: 0.95
// evidence: same as rasterizer_bind_texture_d3d9 but binds through the effect:
//   ID3DXBaseEffect::SetTexture (+0xd0) on effect_slot->effect with the slot's Texture<stage>
//   handle (effect_slot +8 + stage*4). The HRESULT is ignored.
// Spot-check fix (phase 4 review, texture bind helpers 0x518680..0x518a60 rewritten together
//   from the raw code): the earlier rewrite passed NULL where the binary forwards the
//   resolved BitmapData in ESI, called bitmap_group_get_bitmap_data and texture_cache_get
//   without their register arguments (EAX tag / DX frame index, EAX bitmap) and did not pass the bitmap to texture_cache_get.
// register convention: ESI = bitmap, EDI = effect slot, stack = stage; returns a bool in AL (0 only
//   for a NULL bitmap).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"

// blam-cc: EAX -> bitmap
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x00444550

typedef int32_t (__stdcall *d3dx_set_texture_fn)(void *effect, uint32_t handle, void *texture);

// blam-cc: ESI -> bitmap, EDI -> effect_slot, stack -> stage
uint8_t rasterizer_bind_texture_d3dx(int16_t stage, BitmapData *bitmap, rasterizer_effect_slot *effect_slot)
{
    void *effect;

    if (bitmap == 0) {
        return 0;
    }
    texture_cache_get(bitmap, 1, 1);
    effect = (void *)effect_slot->effect;
    ((d3dx_set_texture_fn)(*(void ***)effect)[0xd0 / 4])(effect, effect_slot->texture_handles[stage],
                                                          *(void **)((uint8_t *)bitmap + 0x28));
    return 1;
}

#if 0
Original Ghidra decompilation (0x5186c0):

undefined4 FUN_005186c0(short param_1)

{
  int unaff_ESI;
  undefined4 *unaff_EDI;

  if (unaff_ESI != 0) {
    texture_cache_get(1,1);
    (**(code **)(*(int *)*unaff_EDI + 0xd0))
              ((int *)*unaff_EDI,unaff_EDI[param_1 + 2],*(undefined4 *)(unaff_ESI + 0x28));
    return 1;
  }
  return 0;
}
#endif
