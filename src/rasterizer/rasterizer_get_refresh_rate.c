// rasterizer_get_refresh_rate  (Ghidra: FUN_00515c70, unnamed; named from
// out/phase4/rasterizer_functions.md's summary: "Returns the display refresh rate to use,
// falling back to 60Hz when no enumerated list is available.")
// address 0x515c70, size 42 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: identifies os_platform (types/cache.h) on first use, then only trusts the
//   caller-requested rate (or 0x007c11f8's default) once the platform id is beyond 2; otherwise
//   returns the literal 0x3c (60).
// register convention: requested refresh rate in unaff_ESI (0 = auto).
//   // blam-cc: unaff_ESI -> requested_rate

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern d3d_display_mode rasterizer_desktop_display_mode;           // 0x007c11f0

extern int32_t os_platform;            // 0x00721ef0
extern void os_platform_identify(void); // 0x5427e0

// blam-cc: unaff_ESI -> requested_rate
// Returns `requested_rate` (or the default enumerated rate when it is 0) once the platform has
// been identified as beyond the earliest two ids; otherwise falls back to a fixed 60Hz.
int32_t rasterizer_get_refresh_rate(int32_t requested_rate)
{
    if (os_platform == 0) {
        os_platform_identify();
    }
    if (os_platform > 2) {
        if (requested_rate == 0) {
            return rasterizer_desktop_display_mode.refresh_rate;
        }
        return requested_rate;
    }
    return 0x3c;
}

#if 0
Original Ghidra decompilation (0x515c70):

int FUN_00515c70(void)

{
  int unaff_ESI;

  if (DAT_00721ef0 == 0) {
    os_platform_identify();
  }
  if (2 < DAT_00721ef0) {
    if (unaff_ESI == 0) {
      return DAT_007c11f8;
    }
    return unaff_ESI;
  }
  return 0x3c;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
