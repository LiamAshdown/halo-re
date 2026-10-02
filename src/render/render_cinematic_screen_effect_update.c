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
// reconciled: R11 0x0069c65c..0x0069c668 externs -> rasterizer.h rasterizer_default_z_near/_far + rasterizer_frustum_z_values[2] (float compares/stores kept on the same bits)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern float rasterizer_default_z_near;      // 0x0069c65c, rasterizer.h (R11)
extern float rasterizer_default_z_far;       // 0x0069c660, rasterizer.h (R11)
extern uint32_t rasterizer_frustum_z_values[2]; // 0x0069c664, rasterizer.h: the second {near, far}
                                             //   pair, float bits (0.01171875, 1024.0)

extern void chimera__cinematic_screen_effect(rasterizer_frame_time *time_source); // 0x517470

// Ensures the two default near/far clip distance pairs have been seeded (once, the first time
// either is still exactly 0.0), then dispatches the per-frame cinematic screen effect update.
void render_cinematic_screen_effect_update(rasterizer_frame_time *time_source) // blam-cc: ECX=time_source
{
    if (rasterizer_default_z_near == 0.0f) {
        rasterizer_default_z_near = 0.0625f;
    }
    if (rasterizer_default_z_far == 0.0f) {
        rasterizer_default_z_far = 1024.0f;
    }
    if (*(float *)&rasterizer_frustum_z_values[0] == 0.0f) {
        *(float *)&rasterizer_frustum_z_values[0] = 0.01171875f; // 0x3c400000
    }
    if (*(float *)&rasterizer_frustum_z_values[1] == 0.0f) {
        *(float *)&rasterizer_frustum_z_values[1] = 1024.0f;     // 0x44800000
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
