// rasterizer_apply_decal_zbias  (Ghidra: FUN_005194e0, unnamed; named from
// out/phase4/rasterizer_functions.md's summary: "Applies the decal-pass depth-bias render
// states (0xc3, 0xaf) from a bias table when the corresponding decal-bias flags are set.")
// address 0x5194e0, size 77 bytes
// name confidence: 0.45  rewrite confidence: 0.75
// evidence: same raster_caps gate as rasterizer_clear_decal_zbias.c, applying two undocumented
//   bias value globals instead of zero.
// register convention: none -- no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_device;    // 0x0071d174
extern d3d_caps9 rasterizer_caps;  // 0x007c10c0
extern uint32_t config_decal_z_bias;  // 0x00722b80 UNSURE: depth-bias value
extern uint32_t config_decal_slope_z_bias;  // 0x00722b88 UNSURE: slope-scale-depth-bias value

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

// Applies the decal-pass depth-bias render states (0xc3, 0xaf) from this bias table when the
// corresponding decal-bias flags (raster_caps bits) are set.
void rasterizer_apply_decal_zbias(void)
{
    // the device is only touched inside each caps test: with neither cap set the original never dereferences it
    if ((rasterizer_caps.raster_caps & 0x4000000) != 0) {
        ((d3d_set_render_state_fn)(*(void ***)rasterizer_device)[0xe4 / 4])(rasterizer_device, 0xc3, config_decal_z_bias);
    }
    if ((rasterizer_caps.raster_caps & 0x2000000) != 0) {
        ((d3d_set_render_state_fn)(*(void ***)rasterizer_device)[0xe4 / 4])(rasterizer_device, 0xaf, config_decal_slope_z_bias);
    }
}

#if 0
Original Ghidra decompilation (0x5194e0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_005194e0(void)

{
  if ((_DAT_007c10e4 & 0x4000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc3,DAT_00722b80);
  }
  if ((_DAT_007c10e4 & 0x2000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xaf,DAT_00722b88);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
