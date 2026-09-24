// render_lighting_step_vector3_toward  (Ghidra: FUN_0050f520; new name, evidence below)
// address 0x50f520, size 158 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: sibling of src/render/render_lighting_step_vector4_toward.c (0x50f5c0), same shape
//   for 3 floats instead of 4; out/phase4/render_functions.md's phase-2 summary: "Steps a
//   3-component value toward a target value, clamped to a maximum per-call delta (used to smooth
//   cached lighting samples)." Called from object_render_state_refresh 0x50f270 for
//   render_lighting.ambient, the two distant light directions and shadow_vector (see that
//   sibling's own note on how its call sites were read from disassembly).
// register convention: ECX = current (write), EDX = target (read), stack = max_delta.
//   // blam-cc: ECX -> current, EDX -> target, stack -> max_delta

#include "tags.h"
#include "memory.h"
#include "math.h"

// Steps each of 3 floats in `current` toward the matching float in `target` by at most
// `max_delta`, in place. Used to smooth cached lighting samples frame to frame instead of
// snapping to the freshly sampled value.
void render_lighting_step_vector3_toward(float *current, float *target, float max_delta) // blam-cc: ECX=current, EDX=target, stack=max_delta
{
    float delta;
    float step;
    int i;

    for (i = 0; i < 3; i++) {
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
Original Ghidra decompilation (0x50f520):

void FUN_0050f520(float param_1)

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
    return;
  }
  in_ECX[2] = fVar2 + in_ECX[2];
  return;
}
#endif
