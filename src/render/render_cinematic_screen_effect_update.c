// render_cinematic_screen_effect_update  (Ghidra: FUN_00511df0; new name, evidence below)
// address 0x511df0, size 129 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: types/render.h globals list documents the four defaults this seeds
//   (0x0069c65c/60/64/68, "the default near / far clip pairs 0x511df0 seeds"). Tail-calls
//   chimera__cinematic_screen_effect (0x517470, src/rasterizer/chimera__cinematic_screen_effect.c)
//   via a plain `jmp`, so it forwards its own ECX argument unchanged; objdump of both this
//   function and its two callers (render_frame 0x50bea0, the pregame path 0x50c590, both `lea
//   ecx,[esp+0x10]` immediately before the call) confirms the parameter is the same
//   rasterizer_frame_time block chimera__cinematic_screen_effect already declares in ECX.
// register convention: ECX = time_source (rasterizer_frame_time*), forwarded unchanged.
//   // blam-cc: ECX -> time_source

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern float rasterizer_letterbox_height;    // 0x0069c65c (matches chimera__cinematic_screen_effect.c)
extern float unknown_0069c660;                // 0x0069c660 UNSURE: default far clip pair with the above
extern float unknown_0069c664;                // 0x0069c664 UNSURE: a second near/far clip pair
extern float unknown_0069c668;                // 0x0069c668 UNSURE

extern void chimera__cinematic_screen_effect(rasterizer_frame_time *time_source); // 0x517470

// Ensures the two default near/far clip distance pairs have been seeded (once, the first time
// either is still exactly 0.0), then dispatches the per-frame cinematic screen effect update.
void render_cinematic_screen_effect_update(rasterizer_frame_time *time_source) // blam-cc: ECX=time_source
{
    if (rasterizer_letterbox_height == 0.0f) {
        rasterizer_letterbox_height = 0.0625f;
    }
    if (unknown_0069c660 == 0.0f) {
        unknown_0069c660 = 1024.0f;
    }
    if (unknown_0069c664 == 0.0f) {
        unknown_0069c664 = 0.01171875f;
    }
    if (unknown_0069c668 == 0.0f) {
        unknown_0069c668 = 1024.0f;
    }
    chimera__cinematic_screen_effect(time_source);
}

#if 0
Original Ghidra decompilation (0x511df0):

void FUN_00511df0(void)

{
  if (DAT_0069c65c == 0.0) {
    DAT_0069c65c = 0.0625;
  }
  if (DAT_0069c660 == 0.0) {
    DAT_0069c660 = 1024.0;
  }
  if (DAT_0069c664 == 0.0) {
    DAT_0069c664 = 0.01171875;
  }
  if (DAT_0069c668 == 0.0) {
    DAT_0069c668 = 1024.0;
  }
  chimera__cinematic_screen_effect();
  return;
}

Confirmed as a plain tail jump (register-preserving) via objdump:

00511df0:
  flds   0x672ac0
  flds   0x69c65c
  fucompp
  ...
  jmp    0x517470
#endif
