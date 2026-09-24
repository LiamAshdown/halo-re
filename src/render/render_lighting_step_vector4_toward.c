// render_lighting_step_vector4_toward  (Ghidra: FUN_0050f5c0; new name, evidence below)
// address 0x50f5c0, size 202 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: called from object_render_state_refresh 0x50f270 (objdump) as
//   `mov edx,esi; mov ecx,ebx; call 0x50f5c0` with `lea ecx,[ebx+0x4c]` / `lea edx,[esi+0x4c]`,
//   i.e. destination = &lighting.reflection_tint, source = &desired_lighting.reflection_tint
//   (render_lighting.reflection_tint, types/rasterizer.h, 4 floats at +0x4c). The stack push
//   right before every call site is the max-delta constant (0.03).
// register convention: ECX = current (write), EDX = target (read), stack = max_delta.
//   // blam-cc: ECX -> current, EDX -> target, stack -> max_delta

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

// Steps each of 4 floats in `current` toward the matching float in `target` by at most
// `max_delta`, in place. Used to smooth cached lighting samples (render_lighting.reflection_tint)
// frame to frame instead of snapping to the freshly sampled value.
void render_lighting_step_vector4_toward(float *current, float *target, float max_delta)
{
    float delta;
    float step;
    int i;

    for (i = 0; i < 4; i++) {
        delta = target[i] - current[i];
        step = -max_delta;
        if (-max_delta <= delta) {
            step = delta;
            if (max_delta < delta) {
                step = max_delta;
            }
        }
        current[i] = current[i] + step;
    }
}

#if 0
Original Ghidra decompilation (0x50f5c0):

void FUN_0050f5c0(float param_1)

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
  fVar3 = in_EDX[2] - in_ECX[2];
  fVar1 = fVar2;
  if ((fVar2 <= fVar3) && (fVar1 = fVar3, param_1 < fVar3)) {
    fVar1 = param_1;
  }
  in_ECX[2] = fVar1 + in_ECX[2];
  fVar1 = in_EDX[3] - in_ECX[3];
  if ((fVar2 <= fVar1) && (fVar2 = fVar1, param_1 < fVar1)) {
    in_ECX[3] = param_1 + in_ECX[3];
    return;
  }
  in_ECX[3] = fVar2 + in_ECX[3];
  return;
}
#endif
