// render_device_is_ready  (Ghidra: render_device_is_ready, already named)
// address 0x511d80, size 27 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: types/render.h globals list ("0x0071d16c rasterizer_fullscreen: render_device_is_ready
//   0x511d80 treats it as the 'device initialised' flag (it ANDs the flag with the device
//   pointer)"). __cdecl, no arguments.
// register convention: none (int return).

#include "tags.h"
#include "math.h"
#include "rasterizer.h"

extern uint8_t rasterizer_fullscreen; // 0x0071d16c (matches src/rasterizer/chimera__gamma.c)
extern void *rasterizer_device;       // 0x0071d174 (matches src/rasterizer/chimera__rasterizer_set_framebuffer_blend_function.c)

// Returns whether the rasterizer device has been created and the fullscreen/initialized flag is
// set.
int render_device_is_ready(void)
{
    if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x511d80):

int __cdecl render_device_is_ready(void)

{
  if ((DAT_0071d16c != '\0') && (DAT_0071d174 != 0)) {
    return 1;
  }
  return 0;
}
#endif
