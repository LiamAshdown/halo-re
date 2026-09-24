// rasterizer_end_decal_pass  (Ghidra: FUN_0051b0e0, unnamed; named from
// out/phase4/rasterizer_functions.md's summary: "Cleans up render state after a decal pass,
// clearing the clip-plane flag and z-bias, and restoring shader-stage configuration for stage
// 3.")
// address 0x51b0e0, size 105 bytes
// name confidence: 0.45  rewrite confidence: 0.6
// evidence: clears render state 0x1c (clip-plane enable) and, gated the same way as
//   rasterizer_clear_decal_zbias.c, the two depth-bias states; re-applies
//   rasterizer_set_shader_stage_config when the cached stage tracker reads 3.
// register convention: none -- no parameters.
// UNSURE: 0x006d98dc's role -- not documented in types/rasterizer.h (a different global from the
//   0x0069c6ac shader-stage-config cache the header does list).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device;   // 0x0071d174
extern d3d_caps9 rasterizer_caps; // 0x007c10c0
extern int16_t rasterizer_decal_layer;                              // 0x006d98dc set by rasterizer_decal_pass_begin

// blam-cc: AX -> mode
extern void rasterizer_set_shader_stage_config(int16_t mode);       // 0x519200

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

// Cleans up render state after a decal pass: clears the clip-plane enable state and, where
// active, the depth-bias states, then restores the shader stage configuration if the pass
// tracker reads 3.
void rasterizer_end_decal_pass(void)
{
    void **vtable = *(void ***)rasterizer_device;
    d3d_set_render_state_fn set_render_state = (d3d_set_render_state_fn)vtable[0xe4 / 4];

    set_render_state(rasterizer_device, 0x1c, 0);
    if ((rasterizer_caps.raster_caps & 0x4000000) != 0) {
        set_render_state(rasterizer_device, 0xc3, 0);
    }
    if ((rasterizer_caps.raster_caps & 0x2000000) != 0) {
        set_render_state(rasterizer_device, 0xaf, 0);
    }
    if (rasterizer_decal_layer == 3) {
        rasterizer_set_shader_stage_config(2);
    }
}

#if 0
Original Ghidra decompilation (0x51b0e0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0051b0e0(void)

{
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
  if ((_DAT_007c10e4 & 0x4000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc3,0);
  }
  if ((_DAT_007c10e4 & 0x2000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xaf,0);
  }
  if (DAT_006d98dc == 3) {
    rasterizer_set_shader_stage_config();
    return;
  }
  return;
}
#endif
