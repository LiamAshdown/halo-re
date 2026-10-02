// vector3d_delta_toward_gravity_biased_clamp_length  (Ghidra: FUN_00572a90; renamed for this rewrite)
// address 0x572a90, size 205 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/units_types_notes.md "0x564ae0, 0x5579e0, 0x558860, 0x55eed0, 0x5658f0,
//   0x572a90 -- vector/basis helpers ... blended clamp length". Builds delta = target - origin
//   with a fixed gravity bias added to the z component (0x0069c52c, k_physics_gravity per
//   types/physics.h and src/physics/object_physics_compute_mass_point_forces.c), then clamps
//   that delta's length via vector3d_clamp_length (0x459300, `game`) to a value blended between
//   `max_length_default` and `max_length_aligned` by cos^2(angle between delta and target): the
//   more `target` and the gravity-biased delta point the same way, the closer the clamp gets to
//   `max_length_aligned`. The one caller found (0x574007, `units`, a vehicle hover/lift
//   controller) matches this shape: a per-tick target direction scaled by a lift factor.
// register convention: origin in EAX, target in ECX, output delta in ESI (written in place, also
//   the vector vector3d_clamp_length mutates); max_length_aligned and max_length_default are the
//   two stack parameters, in that order (confirmed by objdump 0x572a90..0x572b5c: `fstp [esp]`
//   is pushed first as the ECX argument to vector3d_clamp_length, i.e. Ghidra's `param_1`).
//   // blam-cc: EAX -> origin, ECX -> target, ESI -> out_delta (out), stack -> (max_length_aligned, max_length_default)
// UNSURE: name and the "blend by alignment" reading are this rewrite's own inference from the
//   arithmetic, not from a string or caller comment; the single caller was only skimmed for
//   plausibility (a vehicle hover/lift target direction), not fully traced.

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern float k_physics_gravity; // 0x0069c52c, ~0.0035651792 world units/tick^2
extern void vector3d_clamp_length(real_vector3d *v, real max_length); // 0x459300, ECX->v, stack->max_length

// Computes out_delta = target - origin (with gravity added to the z component), then clamps its
// length: the more `target` and `out_delta` agree in direction, the closer the clamp is to
// max_length_aligned rather than max_length_default.
void vector3d_delta_toward_gravity_biased_clamp_length(real_point3d *origin, real_point3d *target,
    real_vector3d *out_delta, real max_length_aligned, real max_length_default)
{
    real dot_delta_target;
    real delta_length_squared;
    real target_length_squared;

    out_delta->i = target->x - origin->x;
    out_delta->j = target->y - origin->y;
    out_delta->k = (target->z - origin->z) + k_physics_gravity;

    dot_delta_target = target->y * out_delta->j + out_delta->k * target->z + target->x * out_delta->i;

    if (0.0001f < dot_delta_target) {
        delta_length_squared = out_delta->j * out_delta->j + out_delta->i * out_delta->i + out_delta->k * out_delta->k;
        target_length_squared = target->x * target->x + target->z * target->z + target->y * target->y;
        vector3d_clamp_length(out_delta,
            (max_length_aligned - max_length_default) *
                ((dot_delta_target * dot_delta_target) / delta_length_squared / target_length_squared) +
            max_length_default);
    } else {
        vector3d_clamp_length(out_delta, max_length_default);
    }
}

#if 0
Original Ghidra decompilation (0x572a90):

void FUN_00572a90(float param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float *in_EAX;
  float *in_ECX;
  float *unaff_ESI;

  *unaff_ESI = *in_ECX - *in_EAX;
  unaff_ESI[1] = in_ECX[1] - in_EAX[1];
  fVar2 = (in_ECX[2] - in_EAX[2]) + _DAT_0069c52c;
  unaff_ESI[2] = fVar2;
  fVar1 = in_ECX[1] * unaff_ESI[1] + fVar2 * in_ECX[2] + *in_ECX * *unaff_ESI;
  if (0.0001 < fVar1) {
    vector3d_clamp_length
              ((param_1 - param_2) *
               (((fVar1 * fVar1) /
                (unaff_ESI[1] * unaff_ESI[1] + *unaff_ESI * *unaff_ESI + fVar2 * fVar2)) /
               (*in_ECX * *in_ECX + in_ECX[2] * in_ECX[2] + in_ECX[1] * in_ECX[1])) + param_2);
    return;
  }
  vector3d_clamp_length(param_2);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
