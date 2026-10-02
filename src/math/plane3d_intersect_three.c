// plane3d_intersect_three  (Ghidra: plane3d_intersect_three, already named)
// address 0x4cf040, size 410 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: math_functions.md: "Computes the single point where three planes (param_1, and two
//   register-passed planes) intersect, returning false when the planes are parallel/degenerate."
//   Standard three-plane intersection: p = (d1*(n2 x n3) + d2*(n3 x n1) + d3*(n2 x n1)) /
//   det(n1,n2,n3), det = n1 . (n2 x n3) (vector3d_scalar_triple_product @0x44d8e0, outside this
//   module); degenerate when |det| < 0.0001.
//
// UNSURE: `vector3d_scalar_triple_product` shows only one visible argument (param_1); the other
// two (p2's and p3's normals) are inferred from being the only other vectors this function has,
// used in that order (n2 then n3) to match the rest of the function's EBX-before-EDI ordering.
// Also UNSURE: Ghidra could not decompile the boolean return values cleanly (CONCAT/NAN
// artifacts on both paths); reconstructed as plain 0/1.

// RETURN WIDTH (verified in the disassembly): the success/failure result is written with
// `mov al,1` / `xor al,al` and never zero-extended, so only AL carries the result and the
// return type is a byte, not an int. Declared uint8_t below; reading it as a 32-bit value
// would pick up whatever the upper 24 bits of EAX happened to hold.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double fabs(double x); // ABS is a single x87 FABS instruction
extern real vector3d_scalar_triple_product(const real_vector3d *a, const real_vector3d *b, const real_vector3d *c); // 0x44d8e0, outside this module

// FIXED (register inputs, objdump; one stack argument remains, so no ordering question): the original never reads EAX; p1 arrive(s) on the stack (1 stack argument(s)).
// blam-cc: EBX -> p2, EDI -> p3, ESI -> out, stack -> p1
uint8_t plane3d_intersect_three(real_plane3d *p1, real_plane3d *p2, real_plane3d *p3, real_point3d *out)
{
    real det;
    real abs_det;

    det = vector3d_scalar_triple_product(&p1->normal, &p2->normal, &p3->normal);
    abs_det = (real)fabs((double)det);

    if (abs_det < 0.0001f) {
        return 0;
    }

    {
        real inv_det = 1.0f / det;

        real cross_23_x = p2->normal.j * p3->normal.k - p3->normal.j * p2->normal.k;
        real cross_23_y = p2->normal.k * p3->normal.i - p2->normal.i * p3->normal.k;
        real cross_23_z = p3->normal.j * p2->normal.i - p2->normal.j * p3->normal.i;

        real cross_13_x = p1->normal.k * p3->normal.j - p1->normal.j * p3->normal.k;
        real cross_13_y = p1->normal.i * p3->normal.k - p1->normal.k * p3->normal.i;
        real cross_13_z = p1->normal.j * p3->normal.i - p1->normal.i * p3->normal.j;

        real cross_21_x = p2->normal.k * p1->normal.j - p1->normal.k * p2->normal.j;
        real cross_21_y = p1->normal.k * p2->normal.i - p2->normal.k * p1->normal.i;
        real cross_21_z = p2->normal.j * p1->normal.i - p1->normal.j * p2->normal.i;

        out->x = (cross_23_x * p1->d + cross_13_x * p2->d + cross_21_x * p3->d) * inv_det;
        out->y = (cross_23_y * p1->d + cross_13_y * p2->d + cross_21_y * p3->d) * inv_det;
        out->z = (cross_23_z * p1->d + cross_13_z * p2->d + cross_21_z * p3->d) * inv_det;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4cf040):

undefined4 plane3d_intersect_three(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float10 fVar10;
  float fVar11;
  float fVar12;
  undefined2 extraout_var;
  float *unaff_EBX;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar13;
  float10 fVar14;

  fVar13 = (float10)vector3d_scalar_triple_product(param_1);
  fVar14 = ABS(fVar13);
  fVar10 = (float10)9.999999747378752e-05;
  if (fVar14 >= fVar10) {
    fVar1 = unaff_EBX[2];
    fVar2 = *unaff_EDI;
    fVar3 = *unaff_EBX;
    fVar4 = unaff_EDI[2];
    fVar5 = unaff_EDI[1];
    fVar6 = *unaff_EBX;
    fVar7 = unaff_EBX[1];
    fVar8 = *unaff_EDI;
    fVar9 = param_1[3];
    *unaff_ESI = (unaff_EBX[1] * unaff_EDI[2] - unaff_EDI[1] * unaff_EBX[2]) * fVar9;
    unaff_ESI[1] = (fVar1 * fVar2 - fVar3 * fVar4) * fVar9;
    unaff_ESI[2] = (fVar5 * fVar6 - fVar7 * fVar8) * fVar9;
    fVar1 = *param_1;
    fVar2 = unaff_EDI[2];
    fVar3 = param_1[2];
    fVar4 = *unaff_EDI;
    fVar5 = param_1[1];
    fVar6 = *unaff_EDI;
    fVar7 = unaff_EDI[1];
    fVar8 = *param_1;
    fVar9 = unaff_EBX[3];
    *unaff_ESI = (param_1[2] * unaff_EDI[1] - param_1[1] * unaff_EDI[2]) * fVar9 + *unaff_ESI;
    unaff_ESI[1] = (fVar1 * fVar2 - fVar3 * fVar4) * fVar9 + unaff_ESI[1];
    unaff_ESI[2] = (fVar5 * fVar6 - fVar7 * fVar8) * fVar9 + unaff_ESI[2];
    fVar11 = unaff_EBX[2] * param_1[1] - param_1[2] * unaff_EBX[1];
    fVar1 = param_1[2];
    fVar2 = *unaff_EBX;
    fVar3 = unaff_EBX[2];
    fVar4 = *param_1;
    fVar5 = unaff_EBX[1];
    fVar6 = *param_1;
    fVar7 = *unaff_EBX;
    fVar8 = param_1[1];
    fVar9 = unaff_EDI[3];
    fVar12 = 1.0 / (float)fVar13;
    *unaff_ESI = (fVar11 * fVar9 + *unaff_ESI) * fVar12;
    unaff_ESI[1] = ((fVar1 * fVar2 - fVar3 * fVar4) * fVar9 + unaff_ESI[1]) * fVar12;
    unaff_ESI[2] = ((fVar5 * fVar6 - fVar7 * fVar8) * fVar9 + unaff_ESI[2]) * fVar12;
    return CONCAT31((int3)((uint)fVar11 >> 8),1);
  }
  return CONCAT22(extraout_var,
                  (ushort)(fVar14 < fVar10) << 8 | (ushort)(NAN(fVar14) || NAN(fVar10)) << 10 |
                  (ushort)(fVar14 == fVar10) << 0xe);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
