// rasterizer_transparent_decals_enabled  (Ghidra: rasterizer_transparent_decals_enabled, already
// named)
// address 0x519ac0, size 59 bytes
// name confidence: 0.55  rewrite confidence: 0.6
// evidence: gates on window.type == 1, a debug toggle, and either of the two rasterizer caps
//   flags (0x0069c688/0x0069c68a, types/rasterizer.h) or a ps_1_1-or-better pixel shader
//   version.
// register convention: none -- __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern uint8_t console_debug_toggle_689421;                         // 0x00689421 render target capture enable
extern uint8_t rasterizer_caps_flag_688; // 0x0069c688
extern uint8_t rasterizer_caps_flag_68a; // 0x0069c68a
extern d3d_caps9 rasterizer_caps;     // 0x007c10c0

// Returns whether the current mode and driver capabilities allow transparent-decal/shader-based
// geometry-group rendering to proceed.
int __cdecl rasterizer_transparent_decals_enabled(void)
{
    if (rasterizer_window.type != 1 || console_debug_toggle_689421 == 0 ||
        (rasterizer_caps_flag_688 == 0 &&
         (rasterizer_caps_flag_68a == 0 && rasterizer_caps.pixel_shader_version > 0xffff0100))) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x519ac0):

int __cdecl rasterizer_transparent_decals_enabled(void)

{
  int iVar1;

  iVar1 = 1;
  if ((((short)DAT_007c1220 != 1) || (DAT_00689421 == '\0')) ||
     ((DAT_0069c688 == '\0' && ((DAT_0069c68a == '\0' && (0xffff0100 < DAT_007c118c)))))) {
    iVar1 = 0;
  }
  return iVar1;
}
#endif
