// rasterizer_render_target_bind_effect_texture  (Ghidra: FUN_0052ce10, unnamed)
// address 0x52ce10, size 51 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: `(&DAT_0069d368)[index*5]` is rasterizer_render_targets[index].texture (see
//   rasterizer_render_target_bind_texture_stage.c); vtable+0xd0 is ID3DXBaseEffect::SetTexture,
//   established by rasterizer_bind_texture_d3dx.c's own comment. `in_EDX[param_1+2]` indexes
//   dword 2 onward of the object at EDX, matching rasterizer_effect_slot.texture_handles[4] at
//   +0x08 (dword index 2) exactly, so EDX is a rasterizer_effect_slot* and param_1 selects one of
//   its four texture handles.
// register convention: render target index in AX, effect slot pointer in EDX; texture-handle
//   index is the recognized stack parameter.
//   // blam-cc: AX -> target_index, EDX -> effect_slot, stack -> handle_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358

typedef int32_t (__stdcall *d3dx_effect_settexture_fn)(void *effect, uint32_t handle, void *texture);

// blam-cc: AX -> target_index, EDX -> effect_slot, stack -> handle_index
// Binds a render-target texture to one of an effect's named texture handles instead of a raw
// device sampler stage.
void rasterizer_render_target_bind_effect_texture(int16_t target_index, rasterizer_effect_slot *effect_slot,
                                                    int16_t handle_index)
{
    void *texture = 0;
    void **effect_vtable;

    if (target_index < 9 && target_index > -1) {
        texture = (void *)(uintptr_t)rasterizer_render_targets[target_index].texture;
    }

    effect_vtable = *(void ***)(uintptr_t)effect_slot->effect;
    ((d3dx_effect_settexture_fn)effect_vtable[0xd0 / 4])((void *)(uintptr_t)effect_slot->effect,
        effect_slot->texture_handles[handle_index], texture);
}

#if 0
Original Ghidra decompilation (0x52ce10):

void FUN_0052ce10(short param_1)

{
  short in_AX;
  undefined4 uVar1;
  undefined4 *in_EDX;

  uVar1 = 0;
  if ((in_AX < 9) && (-1 < in_AX)) {
    uVar1 = (&DAT_0069d368)[in_AX * 5];
  }
  (**(code **)(*(int *)*in_EDX + 0xd0))((int *)*in_EDX,in_EDX[param_1 + 2],uVar1);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
