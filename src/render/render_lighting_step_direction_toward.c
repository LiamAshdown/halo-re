// render_lighting_step_direction_toward  (Ghidra: FUN_0050f690; new name, evidence below)
// address 0x50f690, size 172 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: called from object_render_state_refresh 0x50f270 (objdump) as
//   `mov edx,esi; mov ecx,ebx; call 0x50f690` at three sites, with `lea ecx/edx,[reg+0x1c]`,
//   `[reg+0x34]` (render_lighting.distant_light_direction[0]/[1]) and `[reg+0x5c]`
//   (render_lighting.shadow_vector; types/render.h calls out the 0.0015 step and renormalize
//   for this one). Same body as render_lighting_step_vector4_toward 0x50f5c0 but 3 components
//   plus a renormalize, matching a direction rather than a color.
// register convention: ECX = current (write, reused as the normalize argument), EDX = target
//   (read), stack = max_delta.
//   // blam-cc: ECX -> current, EDX -> target, stack -> max_delta

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990

// Steps a 3-component direction toward a target by at most max_delta per component, then
// renormalizes it. Used to smooth cached lighting directions (render_lighting distant light
// directions and shadow_vector) frame to frame instead of snapping to the freshly sampled value.
void render_lighting_step_direction_toward(real_vector3d *current, real_vector3d *target, float max_delta)
{
    float delta;
    float step;
    int i;
    float *cur;
    float *tgt;

    cur = (float *)current;
    tgt = (float *)target;
    for (i = 0; i < 3; i++) {
        delta = tgt[i] - cur[i];
        step = -max_delta;
        if (-max_delta <= delta) {
            step = delta;
            if (max_delta < delta) {
                step = max_delta;
            }
        }
        cur[i] = cur[i] + step;
    }
    vector3d_normalize_with_length(current);
}

#if 0
Original Ghidra decompilation (0x50f690):

void FUN_0050f690(float param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_ECX;
  float *in_EDX;

  fVar1 = *in_EDX - *in_ECX;
  fVar2 = -param_1;
  fVar3 = fVar2;
  if ((fVar2 <= fVar1) && (fVar3 = fVar1, param_1 < fVar1)) {
    fVar3 = param_1;
  }
  *in_ECX = fVar3 + *in_ECX;
  fVar3 = in_EDX[1] - in_ECX[1];
  fVar1 = fVar2;
  if ((fVar2 <= fVar3) && (fVar1 = fVar3, param_1 < fVar3)) {
    fVar1 = param_1;
  }
  in_ECX[1] = fVar1 + in_ECX[1];
  fVar1 = in_EDX[2] - in_ECX[2];
  if ((fVar2 <= fVar1) && (fVar2 = fVar1, param_1 < fVar1)) {
    in_ECX[2] = param_1 + in_ECX[2];
    vector3d_normalize_with_length();
    return;
  }
  in_ECX[2] = fVar2 + in_ECX[2];
  vector3d_normalize_with_length();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
