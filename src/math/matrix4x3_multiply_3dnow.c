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
//   Every PackedFloatingMUL/PackedFloatingADD pair here computes, two floats at a time, one output
//   component as ((b_x * A.forward) + b_y * A.left) + b_z * A.up. This routine is a faster drop-in for
//   `out = a * b` (selected by math_initialize based on the CPU). No SIMD intrinsics are used anywhere
//   else in this codebase, so it is written as scalar C with the exact per-component association of the
//   packed code.
// register convention: __cdecl, all three parameters (a, b, out) on the stack, matching
//   matrix4x3_multiply's recovered signature exactly.
// VERIFIED against disassembly 0x4cc3a0..0x4cc4ff (2026-09-30): FIXED the accumulation order (the earlier text used the
//   Ghidra term order, which differs in the last float bit); the aliasing pre-copy (`a == out` / `b == out`,
//   both into the one scratch buffer) matches 0x4cc3ae..0x4cc3d7.

#include "tags.h"
#include "math.h"

// SSE-optimized implementation of matrix4x3 multiplication, selected at startup when the CPU
// supports the required feature. (Actually AMD 3DNow!; see header.)
void matrix4x3_multiply_3dnow(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out)
{
    real_matrix4x3 scratch;
    const float *A;
    const float *B;
    float *O = (float *)out;
    int32_t column;

    if (a == out) {
        scratch = *a;
        a = &scratch;
    }
    if (b == out) {
        scratch = *b;
        b = &scratch;
    }
    A = (const float *)a; // [0] scale, [1..3] forward, [4..6] left, [7..9] up, [10..12] position
    B = (const float *)b;

    // 0x4cc3f1..0x4cc484: each output axis component is ((bx * A.forward) + by * A.left) + bz * A.up, with every
    // pfmul / pfadd rounded to single precision and accumulated in exactly that order (the first pfadd adds to 0).
    for (column = 0; column < 3; column++) {
        const float bx = B[1 + column * 3];
        const float by = B[2 + column * 3];
        const float bz = B[3 + column * 3];

        O[1 + column * 3] = (bx * A[1] + by * A[4]) + bz * A[7];
        O[2 + column * 3] = (bx * A[2] + by * A[5]) + bz * A[8];
        O[3 + column * 3] = (bx * A[3] + by * A[6]) + bz * A[9];
    }

    // 0x4cc4ac..0x4cc4f3: the translation goes through the same three-term sum, then * a.scale + a.position.
    O[10] = ((B[10] * A[1] + B[11] * A[4]) + B[12] * A[7]) * A[0] + A[10];
    O[11] = ((B[10] * A[2] + B[11] * A[5]) + B[12] * A[8]) * A[0] + A[11];
    O[12] = ((B[10] * A[3] + B[11] * A[6]) + B[12] * A[9]) * A[0] + A[12];
    O[0] = B[0] * A[0];
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
