// rasterizer_render_target_bind_texture_stage  (Ghidra: FUN_0052cdd0, unnamed)
// address 0x52cdd0, size 50 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: `(&DAT_0069d368)[index*5]` is rasterizer_render_targets[index].texture (see
//   rasterizer_light_shadow_render_target_composite.c's file header for the same table/stride);
//   vtable+0x104 is IDirect3DDevice9::SetTexture, established throughout this module.
// register convention: render target index in AX, texture stage in DX.
//   // blam-cc: AX -> target_index, DX -> stage

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"
#include <stdint.h>

extern void *rasterizer_device; // 0x0071d174
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358

typedef int32_t (__stdcall *d3d_set_texture_fn)(void *device, uint32_t stage, void *texture);

// blam-cc: AX -> target_index, DX -> stage
// Binds a previously rendered off-screen render target as a texture on the given sampler stage.
void *rasterizer_render_target_bind_texture_stage(int16_t target_index, int16_t stage)
{
    void *texture = 0;
    void **vtable;

    if (target_index < 9 && target_index > -1) {
        texture = (void *)(uintptr_t)rasterizer_render_targets[target_index].texture;
    }

    vtable = *(void ***)rasterizer_device;
    ((d3d_set_texture_fn)vtable[0x104 / 4])(rasterizer_device, stage, texture);
    return texture;
}

#if 0
Original Ghidra decompilation (0x52cdd0):

undefined4 FUN_0052cdd0(void)

{
  short in_AX;
  short in_DX;
  undefined4 uVar1;

  uVar1 = 0;
  if ((in_AX < 9) && (-1 < in_AX)) {
    uVar1 = (&DAT_0069d368)[in_AX * 5];
  }
  (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,(int)in_DX,uVar1);
  return uVar1;
}
#endif
