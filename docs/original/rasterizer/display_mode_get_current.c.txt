// display_mode_get_current  (Ghidra: display_mode_get_current, already named)
// address 0x515ca0, size 97 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: writes exactly the four fields of rasterizer_display_mode (types/rasterizer.h) from
//   d3d_present_parameters' back_buffer_width/height and fullscreen_refresh_rate, falling back
//   to 60Hz below platform id 3 (same gate as rasterizer_get_refresh_rate.c) and to
//   rasterizer_desktop_display_mode.refresh_rate when the refresh rate field itself is 0; vsync is
//   presentation_interval == 1.
// register convention: destination rasterizer_display_mode* in unaff_EDI.
//   // blam-cc: unaff_EDI -> out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern d3d_display_mode rasterizer_desktop_display_mode;           // 0x007c11f0

extern d3d_present_parameters rasterizer_present_parameters; // 0x007c04a0
extern int32_t os_platform;      // 0x00721ef0
extern void os_platform_identify(void); // 0x5427e0

// blam-cc: unaff_EDI -> out
// Builds a display-mode descriptor from the current cached present parameters, applying the
// same platform-id refresh-rate fallback as rasterizer_get_refresh_rate.c.
void display_mode_get_current(rasterizer_display_mode *out)
{
    int32_t refresh_rate;

    out->width = rasterizer_present_parameters.back_buffer_width;
    out->height = rasterizer_present_parameters.back_buffer_height;
    refresh_rate = rasterizer_present_parameters.fullscreen_refresh_rate;

    if (os_platform == 0) {
        os_platform_identify();
    }

    if (os_platform < 3) {
        refresh_rate = 0x3c;
    } else if (refresh_rate == 0) {
        out->refresh_rate = rasterizer_desktop_display_mode.refresh_rate;
        out->vsync = (rasterizer_present_parameters.presentation_interval == 1);
        return;
    }
    out->refresh_rate = refresh_rate;
    out->vsync = (rasterizer_present_parameters.presentation_interval == 1);
}

#if 0
Original Ghidra decompilation (0x515ca0):

void display_mode_get_current(void)

{
  int iVar1;
  undefined4 *unaff_EDI;
  bool bVar2;

  *unaff_EDI = DAT_007c04a0;
  bVar2 = DAT_00721ef0 == 0;
  unaff_EDI[1] = DAT_007c04a4;
  iVar1 = DAT_007c04d0;
  if (bVar2) {
    os_platform_identify();
  }
  if (DAT_00721ef0 < 3) {
    iVar1 = 0x3c;
  }
  else if (iVar1 == 0) {
    unaff_EDI[2] = DAT_007c11f8;
    *(bool *)(unaff_EDI + 3) = DAT_007c04d4 == 1;
    return;
  }
  unaff_EDI[2] = iVar1;
  *(bool *)(unaff_EDI + 3) = DAT_007c04d4 == 1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
