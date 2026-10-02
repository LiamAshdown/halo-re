// rasterizer_clear_decal_zbias  (Ghidra: rasterizer_clear_decal_zbias, already named)
// address 0x519580, size 67 bytes
// name confidence: 0.55  rewrite confidence: 0.85
// evidence: resets render states 0xc3/0xaf (the same depth-bias pair
//   rasterizer_decal_zbias_active.c tests) to 0 when the matching raster_caps bit is set.
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

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

// Resets the decal depth-bias render states (0xc3, 0xaf) back to zero when the corresponding
// bias flags are active.
void rasterizer_clear_decal_zbias(void)
{
    // the device is only touched inside each caps test (0x51958c, 0x5195ad)
    if ((rasterizer_caps.raster_caps & 0x4000000) != 0) {
        ((d3d_set_render_state_fn)(*(void ***)rasterizer_device)[0xe4 / 4])(rasterizer_device, 0xc3, 0);
    }
    if ((rasterizer_caps.raster_caps & 0x2000000) != 0) {
        ((d3d_set_render_state_fn)(*(void ***)rasterizer_device)[0xe4 / 4])(rasterizer_device, 0xaf, 0);
    }
}

#if 0
Original Ghidra decompilation (0x519580):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void rasterizer_clear_decal_zbias(void)

{
  if ((_DAT_007c10e4 & 0x4000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc3,0);
  }
  if ((_DAT_007c10e4 & 0x2000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xaf,0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
