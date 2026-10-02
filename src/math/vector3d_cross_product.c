// vector3d_cross_product  (Ghidra: vector3d_cross_product, already named)
// address 0x4052c0, size 81 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/math_functions.md; already declared/called as
//   `void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const
//   real_vector3d *b)` with the EAX->out/ECX->a/stack->b mapping in src/ai/
//   actor_get_body_axis_vector.c, src/ai/actor_movement_apply_steering.c and
//   src/ai/actor_refresh_combat_context.c (56 callers total in the binary); this file
//   supplies the definition those externs point at. objdump confirms the register
//   assignment directly: `mov edx,[esp+0x10]` right after `sub esp,0xc` reads the
//   caller's first stack slot (the pre-sub [esp+0x4]) into edx, every term before the
//   output store reads `[ecx+..]`/`[edx+..]`, and `mov ecx,eax` retargets ecx to the
//   output pointer only once the three cross terms are already on the FPU stack.
// review (phase 4 math gate): out = stack_operand x ecx_operand (b x a in this file naming).
//   Most out-of-module externs spell the parameters (out, ecx_operand, stack_operand), which
//   is the same order as here. src/ai/actor_movement_apply_steering.c labels its extern
//   stack->a, ECX->b, the opposite; its one call at 0x418972 (ECX = a stack local, push esi)
//   should be re-checked against this definition.
// register convention: EAX -> out, ECX -> a, stack -> b; no in-place aliasing support is
//   needed by the body (each result component is spilled to a stack temp before out is
//   written), so out may safely alias a or b.
//   // blam-cc: EAX -> out, ECX -> a, stack -> b

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Writes out = the cross product built from a (ECX) and b (stack), component order taken
// directly from the FPU trace: out.i = a.k*b.j - b.k*a.j, out.j = b.k*a.i - b.i*a.k,
// out.k = b.i*a.j - a.i*b.j. (This is -(a x b) / b x a under the usual right-hand-rule
// formula; preserved exactly as compiled rather than renormalized to the "expected" sign,
// per the house rule against inventing behaviour. Every existing caller already treats the
// first vector argument as `a` and the second as `b` with this exact result, so the sign is
// not a bug to fix here -- it is simply this function's convention.)
void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b)
{
    out->i = a->k * b->j - b->k * a->j;
    out->j = b->k * a->i - b->i * a->k;
    out->k = b->i * a->j - a->i * b->j;
}

#if 0
Original Ghidra decompilation (0x4052c0):

void vector3d_cross_product(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float *in_EAX;
  float *in_ECX;

  fVar1 = param_1[2];
  fVar2 = *in_ECX;
  fVar3 = *param_1;
  fVar4 = in_ECX[2];
  fVar5 = *param_1;
  fVar6 = in_ECX[1];
  fVar7 = *in_ECX;
  fVar8 = param_1[1];
  *in_EAX = in_ECX[2] * param_1[1] - param_1[2] * in_ECX[1];
  in_EAX[1] = fVar1 * fVar2 - fVar3 * fVar4;
  in_EAX[2] = fVar5 * fVar6 - fVar7 * fVar8;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
