// chimera__widescreen_text_scaling  (Ghidra: chimera__widescreen_text_scaling, already named --
// Chimera name, hint only)
// address 0x531ab0, size 201 bytes
// name confidence: 0.45   rewrite confidence: 0.9
// evidence: functions.md summary ("Initializes a fixed 5x vec4 vertex-shader constant block
//   (screen-space scale/pixel-alignment values) used when rendering HUD/UI text or the first-person
//   view model"); matches types/rasterizer.h's documented global
//   "0x006e1d08: float rasterizer_ui_text_constants[20] (0x531ab0)" exactly. The first two rows are
//   the same "2/size, -1-1/size" half-pixel-correction shape as rasterizer_screen_flash_render.c's
//   screen transform, but with a fixed 320x240 reference resolution baked in (1/320 = 0.003125,
//   1/240 = 0.0041666...) rather than the current viewport size.
// register convention: none -- __cdecl, no arguments.

#include "tags.h"
#include "math.h"
#include "rasterizer.h"

extern float rasterizer_ui_text_constants[20]; // 0x006e1d08

// Initializes the fixed 5x vec4 vertex-shader constant block (a 320x240-reference-resolution
// screen-space scale/pixel-alignment transform) used when rendering HUD/UI text or the
// first-person view model.
void chimera__widescreen_text_scaling(void)
{
    static const float constants[20] = {
        0.0031250000465661287f, 0.0f, 0.0f, -1.001562476158142f,
        0.0f, -0.004166666883975267f, 0.0f, 1.0020833015441895f,
        0.0f, 0.0f, 0.0f, 0.5f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    int i;
    for (i = 0; i < 20; i++) {
        rasterizer_ui_text_constants[i] = constants[i];
    }
}

#if 0
Original Ghidra decompilation (0x531ab0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void chimera__widescreen_text_scaling(void)

{
  _DAT_006e1d08 = 0x3b4ccccd;
  _DAT_006e1d0c = 0;
  _DAT_006e1d10 = 0;
  _DAT_006e1d14 = 0xbf803333;
  _DAT_006e1d18 = 0;
  _DAT_006e1d1c = 0xbb888889;
  _DAT_006e1d20 = 0;
  _DAT_006e1d24 = 0x3f804444;
  _DAT_006e1d28 = 0;
  _DAT_006e1d2c = 0;
  _DAT_006e1d30 = 0;
  _DAT_006e1d34 = 0x3f000000;
  _DAT_006e1d38 = 0;
  _DAT_006e1d3c = 0;
  _DAT_006e1d40 = 0;
  _DAT_006e1d44 = 0x3f800000;
  _DAT_006e1d48 = 0;
  _DAT_006e1d4c = 0;
  _DAT_006e1d50 = 0;
  _DAT_006e1d54 = 0x3f800000;
  return;
}
#endif
