// effect_random_scaled_range  (Ghidra: FUN_0044c840; renamed here -- out/phase2/results/
// effects_00.json called it "effect_property_random_value", but that name is reassigned to
// 0x451290 by out/phase4/effects_types_notes.md's misattribution table since 0x451290 is the one
// that actually reads Effect.a_scale/b_scale bit-sets. This function is the lower-level generic
// helper 0x451290 (and contrail_generate_points 0x44d020, and other tag readers throughout this
// module) call to turn a {base_min, base_max} bound plus an optional per-bit "multiply by scale"
// flag pair into one random value.)
// address 0x44c840, size 112 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: contrail_generate_points 0x44d020 calls this as
// FUN_0044c840(contrail->scale, Contrail.point_velocity[0], Contrail.point_velocity[1], 1) with
// Contrail.scale_flags (types/tags.h ContrailScaleFlags, bit 1 point_velocity, bit 2
// point_velocity_delta) live in EDX, which is exactly bit_index and bit_index+1.
// register convention: scale-flags bitfield in EDX (in_EDX, held live across a run of calls by
// the caller rather than reloaded from tag data each time); scale, base_min, base_max and
// bit_index on the stack (Ghidra's param_1..param_4).
//   // blam-cc: EDX -> flags, stack -> (scale, base_min, base_max, bit_index)
// UNSURE: the x87 80-bit (float10) intermediate is collapsed to float/double here; every value
// that reaches memory in this module is a 32-bit float, so the extra mantissa bits cannot be
// observed by a caller.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern random_seed effect_random_seed; // 0x00719cd4

// Returns a random value in [base_min, base_max), where base_min and (base_max - base_min) are
// each multiplied by `scale` when their respective flag bit (bit_index and bit_index + 1) is set
// in `flags`.
real effect_random_scaled_range(uint32_t flags, real scale, real base_min, real base_max,
                                uint8_t bit_index)
{
    real lower = base_min;
    real span;

    if ((flags & (1u << (bit_index & 0x1f))) != 0) {
        lower = scale * base_min;
    }
    span = base_max - base_min;
    if ((flags & (1u << ((bit_index + 1) & 0x1f))) != 0) {
        span = span * scale;
    }
    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    return (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * span + lower;
}

#if 0
Original Ghidra decompilation (0x44c840):

float10 FUN_0044c840(float param_1,float param_2,float param_3,byte param_4)

{
  uint in_EDX;
  float10 fVar1;
  float10 fVar2;
  
  fVar1 = (float10)param_2;
  if ((in_EDX & 1 << (param_4 & 0x1f)) != 0) {
    fVar1 = (float10)param_1 * (float10)param_2;
  }
  fVar2 = (float10)param_3 - (float10)param_2;
  if ((in_EDX & 1 << (param_4 + 1 & 0x1f)) != 0) {
    fVar2 = fVar2 * (float10)param_1;
  }
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  return (float10)(DAT_00719cd4 >> 0x10) * (float10)1.5259022e-05 * fVar2 + fVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
