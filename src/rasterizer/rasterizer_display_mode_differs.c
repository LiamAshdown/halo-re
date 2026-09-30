// rasterizer_display_mode_differs  (Ghidra: FUN_00515d10, unnamed; named from
// out/phase4/rasterizer_functions.md's summary: "Determines whether the requested display mode
// differs from the currently active one.")
// address 0x515d10, size 124 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: compares a requested rasterizer_display_mode (width/height/vsync fields match the
//   type header exactly) against rasterizer_present_parameters' dimensions/vsync, and -- only
//   when rasterizer_fullscreen is set and video_force_mode_flag is clear and the dimensions already
//   match -- also compares normalized refresh rates via rasterizer_get_refresh_rate.
// register convention: requested rasterizer_display_mode* in unaff_EDI.
//   // blam-cc: unaff_EDI -> requested
// UNSURE: rasterizer_get_refresh_rate is called twice here with no visible arguments; modeled
//   as comparing the requested mode's refresh_rate against the present parameters' raw
//   fullscreen_refresh_rate, which is the only pairing that makes semantic sense but was not
//   independently confirmed. video_force_mode_flag's meaning is unresolved.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"
#include "fn_shell.h"

extern d3d_present_parameters rasterizer_present_parameters; // 0x007c04a0
extern uint8_t rasterizer_fullscreen;                               // 0x0071d16c (the header called it windowed)
extern uint8_t video_force_mode_flag;      // 0x0071d170 UNSURE
extern int32_t os_platform;           // 0x00721ef0


// blam-cc: unaff_EDI -> requested
// Returns nonzero if `requested` differs from the currently active display mode (dimensions
// always compared; refresh rate compared only while windowed and video_force_mode_flag is clear).
uint8_t rasterizer_display_mode_differs(rasterizer_display_mode *requested)
{
    uint8_t differs = (requested->height != (int32_t)rasterizer_present_parameters.back_buffer_height) ||
                       (requested->width != (int32_t)rasterizer_present_parameters.back_buffer_width);

    if (rasterizer_fullscreen != 0) {
        if (video_force_mode_flag == 0) {
            if (requested->height == (int32_t)rasterizer_present_parameters.back_buffer_height &&
                requested->width == (int32_t)rasterizer_present_parameters.back_buffer_width) {
                if (os_platform == 0) {
                    os_platform_identify();
                }
                if (os_platform > 2) {
                    int32_t rate_a = rasterizer_get_refresh_rate(requested->refresh_rate); // UNSURE
                    int32_t rate_b = rasterizer_get_refresh_rate((int32_t)rasterizer_present_parameters.fullscreen_refresh_rate); // UNSURE
                    differs = (rate_a != rate_b);
                }
            }
        }
        {
            uint8_t vsync_differs = (uint8_t)((rasterizer_present_parameters.presentation_interval == 1) ^ requested->vsync);
            differs = differs | vsync_differs;
        }
    }
    return differs;
}

#if 0
Original Ghidra decompilation (0x515d10):

undefined4 FUN_00515d10(void)

{
  byte bVar1;
  undefined3 uVar4;
  int extraout_EAX;
  int iVar2;
  int iVar3;
  byte bVar5;
  int *unaff_EDI;

  iVar2 = unaff_EDI[1];
  uVar4 = (undefined3)((uint)iVar2 >> 8);
  iVar3 = CONCAT31(uVar4,DAT_0071d16c);
  bVar5 = iVar2 != DAT_007c04a4 || *unaff_EDI != DAT_007c04a0;
  if ((DAT_0071d16c != '\0') && (iVar3 = CONCAT31(uVar4,DAT_0071d170), DAT_0071d170 == '\0')) {
    if (iVar2 == DAT_007c04a4 && *unaff_EDI == DAT_007c04a0) {
      iVar3 = DAT_00721ef0;
      if (DAT_00721ef0 == 0) {
        os_platform_identify();
        iVar3 = extraout_EAX;
      }
      if (2 < DAT_00721ef0) {
        iVar2 = FUN_00515c70();
        iVar3 = FUN_00515c70();
        bVar5 = iVar2 != iVar3;
      }
    }
    bVar1 = DAT_007c04d4 == 1 ^ *(byte *)(unaff_EDI + 3);
    iVar3 = CONCAT31((int3)((uint)iVar3 >> 8),bVar1);
    bVar5 = bVar5 | bVar1;
  }
  return CONCAT31((int3)((uint)iVar3 >> 8),bVar5);
}
#endif
