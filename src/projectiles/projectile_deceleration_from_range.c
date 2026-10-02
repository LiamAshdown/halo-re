// projectile_deceleration_from_range  (Ghidra: FUN_004c03f0; renamed per
// out/phase4/projectiles_types_notes.md: "(v0^2 - v1^2) / (2*(r1 - r0))")
// address 0x4c03f0, size 93 bytes
// name confidence: 0.7   rewrite confidence: 0.9 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: types/tags.h Projectile.initial_velocity (0x1e4), .final_velocity (0x1e8); called
//   only from projectile_compute_deceleration (0x4c0310), which passes one damage-range pair
//   (air or water) as (r0, r1) with the Projectile tag pointer implicit in ECX.
// register convention: ECX = Projectile tag pointer (unaff/implicit, not a Ghidra-recognized
//   parameter); param_1/param_2 (r0, r1) are Ghidra-recognized stack floats.
// blam-cc: ECX -> tag, stack -> (r0, r1)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

// Returns the constant deceleration that would take a projectile from initial_velocity to
// final_velocity over the world-unit span [r0, r1], or 0 when either velocity pair or range
// pair is degenerate.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
real projectile_deceleration_from_range(Projectile *tag, real r0, real r1) // blam-cc: ECX -> tag
{
    real result = 0.0f;

    if (tag->initial_velocity != tag->final_velocity && (r1 - r0) != 0.0f) {
        result = (tag->initial_velocity * tag->initial_velocity -
                  tag->final_velocity * tag->final_velocity) / ((r1 - r0) + (r1 - r0));
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4c03f0):

float10 FUN_004c03f0(float param_1,float param_2)

{
  int in_ECX;
  float10 fVar1;

  fVar1 = (float10)0.0;
  if ((*(float *)(in_ECX + 0x1e4) != *(float *)(in_ECX + 0x1e8)) && (param_2 - param_1 != 0.0)) {
    fVar1 = (float10)(param_2 - param_1);
    fVar1 = ((float10)*(float *)(in_ECX + 0x1e4) * (float10)*(float *)(in_ECX + 0x1e4) -
            (float10)*(float *)(in_ECX + 0x1e8) * (float10)*(float *)(in_ECX + 0x1e8)) /
            (fVar1 + fVar1);
  }
  return fVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
