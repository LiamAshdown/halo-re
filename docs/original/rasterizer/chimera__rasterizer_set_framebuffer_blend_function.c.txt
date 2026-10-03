// chimera__rasterizer_set_framebuffer_blend_function  (Ghidra:
// chimera__rasterizer_set_framebuffer_blend_function, already named -- Chimera name, hint only)
// address 0x5185d0, size 171 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: for blend modes 5/6 with a debug toggle set, forces an additive blend
//   (SrcBlend=DestBlend=2, BlendOp=1); otherwise looks the three render state values up in three
//   parallel .rdata tables indexed by mode.
// register convention: blend mode in in_CX. // blam-cc: CX -> mode
// UNSURE: the three lookup tables' element counts are not bounded by any evidence in this pack;
//   sized generically at 16 entries.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t config_min_max_blend_op_is_broken; // 0x00722b7c
extern void *rasterizer_device;   // 0x0071d174
extern uint32_t rasterizer_blend_src_table[16];  // 0x0065dfbc UNSURE: element count
extern uint32_t rasterizer_blend_dest_table[16]; // 0x0065dfe0 UNSURE: element count
extern uint32_t rasterizer_blend_op_table[16];   // 0x0065e004 UNSURE: element count

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

// blam-cc: CX -> mode
// Configures the framebuffer alpha-blend function (SrcBlend/DestBlend/BlendOp) for the requested
// blend mode, forcing additive blending for modes 5/6 when the debug toggle is set.
void chimera__rasterizer_set_framebuffer_blend_function(int16_t mode)
{
    void **vtable = *(void ***)rasterizer_device;
    d3d_set_render_state_fn set_render_state = (d3d_set_render_state_fn)vtable[0xe4 / 4];

    if (config_min_max_blend_op_is_broken != 0 && (mode == 5 || mode == 6)) {
        set_render_state(rasterizer_device, 0x13, 2);
        set_render_state(rasterizer_device, 0x14, 2);
        set_render_state(rasterizer_device, 0xab, 1);
        return;
    }

    set_render_state(rasterizer_device, 0x13, rasterizer_blend_src_table[mode]);
    set_render_state(rasterizer_device, 0x14, rasterizer_blend_dest_table[mode]);
    set_render_state(rasterizer_device, 0xab, rasterizer_blend_op_table[mode]);
}

#if 0
Original Ghidra decompilation (0x5185d0):

void chimera__rasterizer_set_framebuffer_blend_function(void)

{
  bool bVar1;
  short in_CX;
  int iVar2;

  if ((in_CX == 5) || (in_CX == 6)) {
    bVar1 = true;
  }
  else {
    bVar1 = false;
  }
  if ((DAT_00722b7c != 0) && (bVar1)) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,2);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,2);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
    return;
  }
  iVar2 = in_CX * 4;
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,*(undefined4 *)(&DAT_0065dfbc + iVar2));
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,*(undefined4 *)(&DAT_0065dfe0 + iVar2));
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,*(undefined4 *)(&DAT_0065e004 + iVar2));
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
