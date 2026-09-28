// chimera__transparent_decal_zbias  (Ghidra: chimera__transparent_decal_zbias, already named --
// Chimera name, hint only)
// address 0x519530, size 77 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: same shape as rasterizer_apply_decal_zbias.c with an alternate bias value pair
//   (matches the functions.md summary: "mirroring rasterizer_apply_transparent_decal_zbias for
//   a different bias set").
// register convention: none -- no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device;    // 0x0071d174
extern d3d_caps9 rasterizer_caps;  // 0x007c10c0
extern uint32_t config_transparent_decal_z_bias;  // 0x00722b84 UNSURE: alternate depth-bias value
extern uint32_t config_transparent_decal_slope_z_bias;  // 0x00722b8c UNSURE: alternate slope-scale-depth-bias value

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

// Applies decal-pass depth-bias render states (0xc3/0xaf) from an alternate bias table, for the
// transparent-geometry-group draw path.
void chimera__transparent_decal_zbias(void)
{
    // the device is only touched inside each caps test (0x51953c, 0x519562): with neither cap set the original
    // never dereferences it
    if ((rasterizer_caps.raster_caps & 0x4000000) != 0) {
        ((d3d_set_render_state_fn)(*(void ***)rasterizer_device)[0xe4 / 4])(rasterizer_device, 0xc3, config_transparent_decal_z_bias);
    }
    if ((rasterizer_caps.raster_caps & 0x2000000) != 0) {
        ((d3d_set_render_state_fn)(*(void ***)rasterizer_device)[0xe4 / 4])(rasterizer_device, 0xaf, config_transparent_decal_slope_z_bias);
    }
}

#if 0
Original Ghidra decompilation (0x519530):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void chimera__transparent_decal_zbias(void)

{
  if ((_DAT_007c10e4 & 0x4000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc3,DAT_00722b84);
  }
  if ((_DAT_007c10e4 & 0x2000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xaf,DAT_00722b8c);
  }
  return;
}
#endif
