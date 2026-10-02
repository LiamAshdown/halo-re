// plane3d_intersect_pair_to_line  (Ghidra: plane3d_intersect_pair_to_line, already named)
// address 0x4cf1e0, size 379 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: math_functions.md: "Computes the line of intersection (direction in ECX, a point on
//   it in EDI) of two planes given in EDX and ESI, returning false if the planes are parallel."
//   direction = cross(p1.normal, p2.normal); point = (d1*cross(dir,n2) + d2*cross(n1,dir)) /
//   |dir|^2, the standard two-plane-to-line formula; degenerate when |dir|^2 (compared via
//   ABS(), effectively just the value itself since it's already a sum of squares) < 0.0001.
// register convention: direction-output pointer in ECX (in_ECX), second plane pointer in EDX
//   (in_EDX), first plane pointer in ESI (unaff_ESI), point-output pointer in EDI
//   (unaff_EDI), no stack arguments.
//   // blam-cc: ECX -> direction_out, EDX -> p2, ESI -> p1, EDI -> point_out
//
// UNSURE: Ghidra could not decompile the boolean return values cleanly (CONCAT/NAN artifacts on
// both paths); reconstructed as plain 0/1.

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

uint8_t plane3d_intersect_pair_to_line(real_vector3d *direction_out, real_plane3d *p2, real_plane3d *p1, real_point3d *point_out)
{
    real len2;

    direction_out->i = p2->normal.k * p1->normal.j - p1->normal.k * p2->normal.j;
    direction_out->j = p1->normal.k * p2->normal.i - p1->normal.i * p2->normal.k;
    direction_out->k = p1->normal.i * p2->normal.j - p2->normal.i * p1->normal.j;

    len2 = direction_out->k * direction_out->k + direction_out->j * direction_out->j + direction_out->i * direction_out->i;
    if ((real)fabs((double)len2) < 0.0001f) {
        return 0;
    }

    {
        real point_x_partial = (direction_out->k * p2->normal.j - direction_out->j * p2->normal.k) * p1->d;
        real point_y_partial = (p2->normal.k * direction_out->i - direction_out->k * p2->normal.i) * p1->d;
        real point_z_partial = (direction_out->j * p2->normal.i - direction_out->i * p2->normal.j) * p1->d;
        real inv_len2 = 1.0f / len2;

        point_out->x = ((p1->normal.k * direction_out->j - direction_out->k * p1->normal.j) * p2->d + point_x_partial) * inv_len2;
        point_out->y = ((direction_out->k * p1->normal.i - p1->normal.k * direction_out->i) * p2->d + point_y_partial) * inv_len2;
        point_out->z = ((direction_out->i * p1->normal.j - direction_out->j * p1->normal.i) * p2->d + point_z_partial) * inv_len2;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4cf1e0):

uint plane3d_intersect_pair_to_line(void)

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
  float fVar10;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;
  float *unaff_EDI;

  fVar1 = unaff_ESI[2];
  fVar2 = *in_EDX;
  fVar3 = *unaff_ESI;
  fVar4 = in_EDX[2];
  fVar5 = *unaff_ESI;
  fVar6 = in_EDX[1];
  fVar7 = *in_EDX;
  fVar8 = unaff_ESI[1];
  *in_ECX = in_EDX[2] * unaff_ESI[1] - unaff_ESI[2] * in_EDX[1];
  in_ECX[1] = fVar1 * fVar2 - fVar3 * fVar4;
  in_ECX[2] = fVar5 * fVar6 - fVar7 * fVar8;
  fVar1 = in_ECX[2] * in_ECX[2] + in_ECX[1] * in_ECX[1] + *in_ECX * *in_ECX;
  fVar2 = ABS(fVar1);
  if (fVar2 >= 0.0001) {
    fVar2 = in_EDX[2];
    fVar3 = *in_ECX;
    fVar4 = in_ECX[2];
    fVar5 = *in_EDX;
    fVar6 = in_ECX[1];
    fVar7 = *in_EDX;
    fVar8 = *in_ECX;
    fVar9 = in_EDX[1];
    fVar10 = unaff_ESI[3];
    *unaff_EDI = (in_ECX[2] * in_EDX[1] - in_ECX[1] * in_EDX[2]) * fVar10;
    unaff_EDI[1] = (fVar2 * fVar3 - fVar4 * fVar5) * fVar10;
    unaff_EDI[2] = (fVar6 * fVar7 - fVar8 * fVar9) * fVar10;
    fVar7 = in_ECX[2] * *unaff_ESI - unaff_ESI[2] * *in_ECX;
    fVar2 = *in_ECX;
    fVar3 = unaff_ESI[1];
    fVar4 = in_ECX[1];
    fVar5 = *unaff_ESI;
    fVar6 = in_EDX[3];
    fVar1 = 1.0 / fVar1;
    *unaff_EDI = ((unaff_ESI[2] * in_ECX[1] - in_ECX[2] * unaff_ESI[1]) * fVar6 + *unaff_EDI) *
                 fVar1;
    unaff_EDI[1] = (fVar7 * fVar6 + unaff_EDI[1]) * fVar1;
    unaff_EDI[2] = ((fVar2 * fVar3 - fVar4 * fVar5) * fVar6 + unaff_EDI[2]) * fVar1;
    return CONCAT31((int3)((uint)fVar7 >> 8),1);
  }
  return (uint)(ushort)((ushort)(fVar2 < 0.0001) << 8 | (ushort)NAN(fVar2) << 10 |
                       (ushort)(fVar2 == 0.0001) << 0xe);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
