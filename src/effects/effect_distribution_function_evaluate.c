// effect_distribution_function_evaluate  (Ghidra: transition_function_evaluate; renamed here to
//   avoid colliding with the unrelated math module function of the same Ghidra name at 0x4ccac0)
// address 0x453290, size 124 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: types/tags.h EffectDistributionFunction (start/end/constant/buildup/falloff/
//   buildup_and_falloff, values 0..5) matches every switch case's arithmetic: start always 1,
//   end is a 0/1 step at fraction 1, constant is the identity, buildup is fraction^2, falloff is
//   (2-fraction)*fraction (== 1-(1-fraction)^2), buildup_and_falloff is the cubic smoothstep
//   (3-2*fraction)*fraction^2. out/phase4/effects_types_notes.md: "the EffectDistributionFunction
//   CDF, not a general easing curve."
// register convention: none -- both arguments are Ghidra-recognized stack parameters.

#include "tags.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Evaluates the cumulative distribution function of an EffectDistributionFunction at `fraction`
// (0..1, or -1.0 as a sentinel that always yields 0). effect_spawn_particles takes the difference
// of two evaluations (this tick's event fraction minus last tick's) to get a per-tick spawn
// weight, which is why "end" must be a hard step and "start" must be flat at 1.
float effect_distribution_function_evaluate(EffectDistributionFunction_t type, float fraction)
{
    if (fraction == -1.0f) {
        return 0.0f;
    }
    switch (type) {
    case effectdistributionfunction_start:
        return 1.0f;
    case effectdistributionfunction_end:
        return (fraction < 1.0f) ? 0.0f : 1.0f;
    case effectdistributionfunction_buildup:
        return fraction * fraction;
    case effectdistributionfunction_falloff:
        return (2.0f - fraction) * fraction;
    case effectdistributionfunction_buildup_and_falloff:
        return (3.0f - (fraction + fraction)) * fraction * fraction;
    default: // effectdistributionfunction_constant, and any other value
        return fraction;
    }
}

#if 0
Original Ghidra decompilation (0x453290):

float10 transition_function_evaluate(undefined2 param_1,float param_2)

{
  if (param_2 == -1.0) {
    return (float10)0.0;
  }
  switch(param_1) {
  case 0:
    goto switchD_004532b4_caseD_0;
  case 1:
    if (param_2 < 1.0) {
      return (float10)0.0;
    }
switchD_004532b4_caseD_0:
    return (float10)1.0;
  default:
    return (float10)param_2;
  case 3:
    return (float10)param_2 * (float10)param_2;
  case 4:
    return ((float10)2.0 - (float10)param_2) * (float10)param_2;
  case 5:
    return ((float10)3.0 - ((float10)param_2 + (float10)param_2)) * (float10)param_2 *
           (float10)param_2;
  }
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
