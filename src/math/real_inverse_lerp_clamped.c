// real_inverse_lerp_clamped  (Ghidra: FUN_00507430; renamed)
// address 0x507430, size 123 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: out/phase4/physics_types_notes.md "0x507430 - a clamped inverse lerp of a value
//   between two references, returning 0 or 1 outside. Pure math, one caller inside [physics]
//   and one in 0x509e80. No types."; both callers (src/physics/object_physics_compute_mass_point_forces.c
//   and src/physics/object_physics_tick_single_pass.c) already declare
//   `extern float FUN_00507430(float value, float ref_k0, float ref_k1);` and call it to turn a
//   mass point's up/resting-plane k component into a clamped 0..1 "lean" fraction between two
//   ground/antigrav normal k references. Confirmed against
//   `objdump -d -M intel --start-address=0x507430 --stop-address=0x5074ab bin/halo.exe`: three
//   pure stack arguments ([esp+4]/[esp+8]/[esp+0xc]), no register-passed arguments, x87 return.
//   cleanup pass 4 orphan pass: physics judged this address out of place for physics (see
//   out/phase4/orphans_notes.md) and it was picked up here.
// register convention: __cdecl, all three arguments on the stack, no hidden registers.
//   // blam-cc: stack -> (value, ref_k0, ref_k1)
// UNSURE: the two comparisons the compiler wrote as `(value < ref) != (value == ref)` are its
//   unordered-compare-safe way of expressing `value <= ref` (or `>=`, mirrored) for non-NaN
//   inputs; kept in that exact form rather than folded to <=/>= so a NaN input takes the same
//   path here as in the original binary.

#include "tags.h"
#include "math.h"

// Returns how far value sits from ref_k0 towards ref_k1, as a fraction clamped to [0, 1].
// ref_k0 and ref_k1 may be given in either order; the "near" end (ref_k0) maps to 0 and the
// "far" end (ref_k1) maps to 1, with values beyond ref_k1 clamped to 1 and values on the
// ref_k0 side of ref_k0 clamped to 0.
real real_inverse_lerp_clamped(real value, real ref_k0, real ref_k1)
{
    if (ref_k1 <= ref_k0) {
        if ((value < ref_k1) != (value == ref_k1)) {
            return 1.0f;
        }
        if (value < ref_k0) {
            return (ref_k0 - value) / (ref_k0 - ref_k1);
        }
    } else if ((value < ref_k0) == (value == ref_k0)) {
        if (value < ref_k1) {
            return (value - ref_k0) / (ref_k1 - ref_k0);
        }
        return 1.0f;
    }
    return 0.0f;
}

#if 0
Original Ghidra decompilation (0x507430):

float10 FUN_00507430(float param_1,float param_2,float param_3)

{
  if (param_3 <= param_2) {
    if (param_1 < param_3 != (param_1 == param_3)) goto LAB_00507464;
    if (param_1 < param_2) {
      return ((float10)param_2 - (float10)param_1) / ((float10)param_2 - (float10)param_3);
    }
  }
  else if (param_1 < param_2 == (param_1 == param_2)) {
    if (param_1 < param_3) {
      return ((float10)param_1 - (float10)param_2) / ((float10)param_3 - (float10)param_2);
    }
LAB_00507464:
    return (float10)1.0;
  }
  return (float10)0.0;
}
#endif
