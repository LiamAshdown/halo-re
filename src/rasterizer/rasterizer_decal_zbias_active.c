// rasterizer_decal_zbias_active  (Ghidra: rasterizer_decal_zbias_active, already named)
// address 0x5195d0, size 21 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: tests the same two raster_caps bits (0x04000000 DEPTHBIAS, 0x02000000
//   SLOPESCALEDEPTHBIAS) that gate render states 0xc3/0xaf throughout this module
//   (types/rasterizer.h).
// register convention: none -- __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern d3d_caps9 rasterizer_caps; // 0x007c10c0

// Returns whether either decal depth-bias flag is currently set in raster_caps.
int __cdecl rasterizer_decal_zbias_active(void)
{
    if ((rasterizer_caps.raster_caps & 0x6000000) == 0) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x5195d0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __cdecl rasterizer_decal_zbias_active(void)

{
  if ((_DAT_007c10e4 & 0x6000000) == 0) {
    return 0;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
