// physics_scalar_advance_and_wrap  (Ghidra: FUN_0050b290, still unnamed there; name chosen to
//   match this batch's other physics_scalar_* helpers)
// address 0x50b290, size 86 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/physics_functions.md summary ("Advances a scalar physics state value by
//   a delta and clamps or wraps it to stay within a given range"); types/physics.h
//   physics_scalar_range comment ("read UPPER FIRST: the code computes the span as
//   range[0] - range[1] and clamps the low side against range[1]"), which is exactly the
//   in_ECX[1] <= value / *in_ECX < value pair of tests below.
// register convention: in_ECX -> range (physics_scalar_range *), in_EDX -> value (float *,
//   read-modify-write), unaff_BL -> wrap (nonzero: wrap the overshoot back into the range
//   instead of clamping to the edge). param_1 is Ghidra's own recognized stack parameter
//   (delta).
//   // blam-cc: ECX -> range, EDX -> value, BL -> wrap, stack -> delta
// UNSURE: ownership of this whole physics_scalar_* family (0x50b290/b2f0/b370/b460/b4d0) is
//   unresolved -- out/phase4/physics_types_notes.md section 5 flags them as more likely
//   turret/vehicle aiming code than rigid-body physics, with a single caller each outside this
//   module's slice.

#include "tags.h"
#include "math.h"
#include "physics.h"

// Adds delta to *value, then keeps the result inside [range->lower, range->upper]: overshooting
// the upper edge either wraps by the range's span (wrap != 0) or clamps to range->upper;
// undershooting the lower edge does the mirror image against range->lower.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void physics_scalar_advance_and_wrap(physics_scalar_range *range, float *value, uint8_t wrap, float delta)
{
    float new_value = *value + delta;
    *value = new_value;

    if (range->lower <= new_value) {
        if (range->upper < new_value) {
            if (wrap != 0) {
                *value = new_value - (range->upper - range->lower);
                return;
            }
            *value = range->upper;
        }
        return;
    }

    if (wrap != 0) {
        *value = (range->upper - range->lower) + new_value;
        return;
    }
    *value = range->lower;
}

#if 0
Original Ghidra decompilation (0x50b290):

void FUN_0050b290(float param_1)

{
  float *in_ECX;
  float *in_EDX;
  char unaff_BL;

  param_1 = param_1 + *in_EDX;
  *in_EDX = param_1;
  if (in_ECX[1] <= param_1) {
    if (*in_ECX < param_1) {
      if (unaff_BL != '\0') {
        *in_EDX = param_1 - (*in_ECX - in_ECX[1]);
        return;
      }
      *in_EDX = *in_ECX;
    }
    return;
  }
  if (unaff_BL != '\0') {
    *in_EDX = (*in_ECX - in_ECX[1]) + param_1;
    return;
  }
  *in_EDX = in_ECX[1];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
