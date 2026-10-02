// vector3d_scalar_triple_product  (already named; task-provided)
// address 0x44d8e0, size 107 bytes
// name confidence: 0.6 (already carries this name; computes (b x c) . a via the textbook
//   scalar triple product formula, matching src/effects/README.md and
//   src/math/README.md's "Misattributed functions" note)
// rewrite confidence: 0.6 (pure arithmetic, confirmed against the decompilation)
// evidence: src/math/README.md, "0x44d8e0 is vector3d_scalar_triple_product", "generic;
//   all three operands in registers". out/phase4/effects_types_notes.md agrees.
// register convention: EAX = const real_vector3d *b, EDX = const real_vector3d *c; a is the one
//   Ghidra-recognized stack parameter (param_1) -- objdump shows it loaded from [esp+0x1c] into
//   ECX right at entry (a local cache of the stack slot, not an incoming ECX register value).
//   // blam-cc: EAX -> b, EDX -> c, stack -> a
// FIXED (register inputs, objdump): EAX carries b (read at 0x44d8e7, fld [eax+0x8]) and EDX
// carries c (read at 0x44d933, fmul [edx+0x8]); the old note wrote them in an inline-comment
// form ("real_vector3d *b /*EAX*/") the checker's parser does not recognize, and additionally
// misclassified a as an ECX register when it is really the sole stack argument, merely cached
// into ECX by the prologue.
// UNSURE: Ghidra's float10 return is the x87 calling convention for a float result;
//   rewritten as float, matching the convention used throughout this codebase (e.g.
//   src/ai/actor_compute_target_priority_weight.c).

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

float vector3d_scalar_triple_product(const real_vector3d *a, const real_vector3d *b, const real_vector3d *c)
{
    return (b->k * a->j - a->k * b->j) * c->i +
           (a->k * b->i - a->i * b->k) * c->j +
           (a->i * b->j - b->i * a->j) * c->k;
}

#if 0
Original Ghidra decompilation (0x44d8e0):

float10 vector3d_scalar_triple_product(float *param_1)

{
  float *in_EAX;
  float *in_EDX;

  return (float10)(in_EAX[2] * param_1[1] - param_1[2] * in_EAX[1]) * (float10)*in_EDX +
         (float10)(param_1[2] * *in_EAX - *param_1 * in_EAX[2]) * (float10)in_EDX[1] +
         (float10)(*param_1 * in_EAX[1] - *in_EAX * param_1[1]) * (float10)in_EDX[2];
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
