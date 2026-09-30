// matrix4x3_multiply_3dnow  (Ghidra: matrix4x3_multiply_sse; renamed per evidence below)
// address 0x4cc3a0, size 352 bytes
// name confidence: 0.85   rewrite confidence: 0.6
// evidence: out/phase4/math_types_notes.md item 1: this function's body contains 21 `0f 0f`
//   escape-byte opcodes (PFMUL/PFADD, which Ghidra renders as PackedFloatingMUL/
//   PackedFloatingADD) and zero SSE opcodes -- it is AMD 3DNow!, not SSE, despite Ghidra's
//   auto-generated name. The genuine SSE implementation is the unnamed function at 0x004cc250
//   (MOVSS/MOVHPS/SHUFPS), which is outside this module's 90-function list and is not rewritten
//   here. math_initialize @0x4cd3f0 installs this routine via
//   matrix4x3_multiply_procedure when cpu_get_type(0x1a) reports 3DNow! support.
//   Every PackedFloatingMUL/PackedFloatingADD pair here computes, two floats at a time, exactly
//   the same sums that matrix4x3_multiply @0x4cc0d0 computes with scalar multiplies and adds in
//   the same accumulation order (three terms per output component, accumulated 0/1/2). Since
//   this routine exists purely as a faster drop-in for that same `out = a * b` operation (the
//   two are selected interchangeably by math_initialize based on the CPU), it is rewritten here
//   with the identical scalar formula rather than hand-decoded packed-pair arithmetic: the
//   observable behaviour (the output matrix written to `out`) is the same either way, and no
//   SIMD intrinsics are used anywhere else in this codebase.
// register convention: __cdecl, all three parameters (a, b, out) on the stack, matching
//   matrix4x3_multiply's recovered signature exactly.
// UNSURE: this is a semantic-equivalence rewrite, not a mechanical one -- see above. The
//   aliasing pre-copy (`a == out` / `b == out`) is preserved faithfully from the decompile.

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// SSE-optimized implementation of matrix4x3 multiplication, selected at startup when the CPU
// supports the required feature. (Actually AMD 3DNow!; see header.)
void matrix4x3_multiply_3dnow(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out)
{
    real_matrix4x3 scratch;

    if (a == out) {
        scratch = *a;
        a = &scratch;
    }
    if (b == out) {
        scratch = *b;
        b = &scratch;
    }

    out->forward.i = b->forward.k * a->up.i + a->forward.i * b->forward.i + a->left.i * b->forward.j;
    out->forward.j = a->left.j * b->forward.j + a->forward.j * b->forward.i + a->up.j * b->forward.k;
    out->forward.k = a->left.k * b->forward.j + a->forward.k * b->forward.i + a->up.k * b->forward.k;
    out->left.i = a->left.i * b->left.j + b->left.k * a->up.i + a->forward.i * b->left.i;
    out->left.j = a->forward.j * b->left.i + a->left.j * b->left.j + a->up.j * b->left.k;
    out->left.k = a->forward.k * b->left.i + a->left.k * b->left.j + a->up.k * b->left.k;
    out->up.i = a->left.i * b->up.j + a->forward.i * b->up.i + b->up.k * a->up.i;
    out->up.j = b->up.j * a->left.j + b->up.k * a->up.j + a->forward.j * b->up.i;
    out->up.k = b->up.j * a->left.k + b->up.k * a->up.k + a->forward.k * b->up.i;
    out->position.x = (b->position.z * a->up.i + a->left.i * b->position.y + a->forward.i * b->position.x) * a->scale + a->position.x;
    out->position.y = (b->position.x * a->forward.j + b->position.y * a->left.j + a->up.j * b->position.z) * a->scale + a->position.y;
    out->position.z = (b->position.x * a->forward.k + b->position.y * a->left.k + a->up.k * b->position.z) * a->scale + a->position.z;
    out->scale = a->scale * b->scale;
}

#if 0
Original Ghidra decompilation (0x4cc3a0):

void __cdecl matrix4x3_multiply_sse(float *a,float *b,float *out)

{
  float *pfVar1;
  int iVar2;
  float *pfVar3;
  ulonglong uVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  undefined8 uVar7;
  undefined8 uVar8;
  undefined8 uVar9;
  float local_38 [13];

  if (a == out) {
    pfVar1 = local_38;
    for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
      *pfVar1 = *a;
      a = a + 1;
      pfVar1 = pfVar1 + 1;
    }
    a = local_38;
  }
  if (b == out) {
    pfVar1 = b;
    pfVar3 = local_38;
    for (iVar2 = 0xd; b = local_38, iVar2 != 0; iVar2 = iVar2 + -1) {
      *pfVar3 = *pfVar1;
      pfVar1 = pfVar1 + 1;
      pfVar3 = pfVar3 + 1;
    }
  }
  iVar2 = 3;
  uVar7 = 0;
  uVar8 = 0;
  uVar9 = 0;
  pfVar1 = a;
  pfVar3 = b;
  do {
    uVar6 = *(undefined8 *)(pfVar1 + 1);
    uVar5 = PackedFloatingMUL(CONCAT44(pfVar3[1],pfVar3[1]),uVar6);
    uVar7 = PackedFloatingADD(uVar7,uVar5);
    uVar5 = PackedFloatingMUL(CONCAT44(pfVar3[4],pfVar3[4]),uVar6);
    uVar6 = PackedFloatingMUL(CONCAT44(pfVar3[7],pfVar3[7]),uVar6);
    pfVar1 = pfVar1 + 3;
    uVar8 = PackedFloatingADD(uVar8,uVar5);
    pfVar3 = pfVar3 + 1;
    uVar9 = PackedFloatingADD(uVar9,uVar6);
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  *(undefined8 *)(out + 1) = uVar7;
  *(undefined8 *)(out + 4) = uVar8;
  *(undefined8 *)(out + 7) = uVar9;
  iVar2 = 3;
  uVar7 = 0;
  uVar8 = 0;
  uVar9 = 0;
  pfVar1 = a;
  pfVar3 = b;
  do {
    uVar4 = (ulonglong)(uint)pfVar1[3];
    uVar6 = PackedFloatingMUL((ulonglong)(uint)pfVar3[1],uVar4);
    uVar5 = PackedFloatingMUL((ulonglong)(uint)pfVar3[4],uVar4);
    uVar7 = PackedFloatingADD(uVar7,uVar6);
    uVar6 = PackedFloatingMUL((ulonglong)(uint)pfVar3[7],uVar4);
    pfVar1 = pfVar1 + 3;
    uVar8 = PackedFloatingADD(uVar8,uVar5);
    pfVar3 = pfVar3 + 1;
    uVar9 = PackedFloatingADD(uVar9,uVar6);
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  out[3] = (float)uVar7;
  out[6] = (float)uVar8;
  out[9] = (float)uVar9;
  iVar2 = 3;
  uVar9 = 0;
  uVar6 = 0;
  uVar7 = CONCAT44(*a,*a);
  uVar8 = PackedFloatingMUL((ulonglong)(uint)*b,uVar7);
  *out = (float)uVar8;
  pfVar1 = a;
  do {
    uVar5 = CONCAT44(b[10],b[10]);
    uVar8 = PackedFloatingMUL(*(undefined8 *)(pfVar1 + 1),uVar5);
    uVar5 = PackedFloatingMUL((ulonglong)(uint)pfVar1[3],uVar5);
    uVar9 = PackedFloatingADD(uVar9,uVar8);
    uVar6 = PackedFloatingADD(uVar6,uVar5);
    pfVar1 = pfVar1 + 3;
    b = b + 1;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  uVar8 = PackedFloatingMUL(uVar9,uVar7);
  uVar9 = PackedFloatingMUL(uVar6,uVar7);
  uVar7 = PackedFloatingADD(uVar8,*(undefined8 *)(a + 10));
  uVar8 = PackedFloatingADD(uVar9,(ulonglong)(uint)a[0xc]);
  *(undefined8 *)(out + 10) = uVar7;
  out[0xc] = (float)uVar8;
  return;
}
#endif
