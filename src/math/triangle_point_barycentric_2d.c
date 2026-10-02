// triangle_point_barycentric_2d  (Ghidra: FUN_004ce8c0; renamed, no established name)
// address 0x4ce8c0, size 544 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: math_functions.md: "Given a triangle (EAX,ECX,EDX) and a point on its plane (ESI),
//   computes two barycentric-style coordinates (param_1,param_2) describing the point's
//   position on the triangle, using axis-dropping 2D projection." Drops the plane normal's
//   dominant axis (via out/phase4/math_types_notes.md's k_projection_axes table @0x0065c29c)
//   and solves the resulting 2D system e0 = u*e1 + v*e2 (e1 = v_edx-a, e2 = v_ecx-a,
//   e0 = p-a) with 2D cross products.
// register convention: origin vertex pointer in EAX (in_EAX), second edge's vertex pointer in
//   ECX (in_ECX), first edge's vertex pointer in EDX (in_EDX), point pointer in ESI
//   (unaff_ESI); u/v output pointers as the recognized stack parameters (param_1, param_2).
//   // blam-cc: EAX -> a, ECX -> v_ecx, EDX -> v_edx, ESI -> p, stack -> (out_u, out_v)
//
// UNSURE: Ghidra could not decompile the actual failure-path return values -- every early-out
// prints raw CONCAT2/CONCAT3 register-half packing and NAN()-flag expressions instead of a
// clean value (the same known Ghidra failure as ray_intersects_sphere_test @0x4ce6c0).
// Reconstructed as a plain 0 (only the success path's return 1 is a literal decompiled value).
// Also UNSURE: `local_30`/`local_40` are indexed past their Ghidra-declared bounds
// (`local_30[iVar7 + 6]` etc.) in a way that only makes sense as three separate contiguous
// 3-float locals (e2, then e0) laid out immediately after the declared arrays -- the same kind
// of stack-splitting artifact seen in periodic_function_build_table @0x4ccdb0. Rewritten here
// as three explicit 3-float arrays (e0, e1, e2) rather than reproducing the overlapping layout.

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
extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c

uint8_t triangle_point_barycentric_2d(real_point3d *a, real_point3d *v_ecx, real_point3d *v_edx, real_point3d *p, real *out_u, real *out_v)
{
    real e0[3]; // p - a
    real e1[3]; // v_edx - a
    real e2[3]; // v_ecx - a
    real n[3];  // cross(e1, e2), the (non-unit) triangle normal
    real dot_n_e0;
    real plane_tolerance;
    int dominant_axis;
    int axis_index;
    int i;
    int j;
    real e1_i, e1_j;
    real e0_i, e0_j;
    real denom_cross;
    real numerator;
    real denom;

    e0[0] = p->x - a->x; e0[1] = p->y - a->y; e0[2] = p->z - a->z;
    e1[0] = v_edx->x - a->x; e1[1] = v_edx->y - a->y; e1[2] = v_edx->z - a->z;
    e2[0] = v_ecx->x - a->x; e2[1] = v_ecx->y - a->y; e2[2] = v_ecx->z - a->z;

    n[0] = e2[2] * e1[1] - e2[1] * e1[2];
    n[1] = e1[2] * e2[0] - e2[2] * e1[0];
    n[2] = e2[1] * e1[0] - e1[1] * e2[0];

    dot_n_e0 = n[0] * e0[0] + n[2] * e0[2] + n[1] * e0[1];
    dot_n_e0 = dot_n_e0 * dot_n_e0;
    plane_tolerance = (n[0] * n[0] + n[1] * n[1] + n[2] * n[2]) * 0.0001f;

    if (plane_tolerance <= dot_n_e0) {
        return 0; // point is not close enough to the triangle's plane
    }

    if ((real)fabs((double)n[2]) < (real)fabs((double)n[1]) || (real)fabs((double)n[2]) < (real)fabs((double)n[0])) {
        if ((real)fabs((double)n[1]) < (real)fabs((double)n[0])) {
            dominant_axis = 0;
        } else {
            dominant_axis = 1;
        }
    } else {
        dominant_axis = 2;
    }

    axis_index = (0.0f < n[dominant_axis] ? 1 : 0) + dominant_axis * 2;
    i = k_projection_axes[axis_index].i;
    j = k_projection_axes[axis_index].j;

    e1_i = e1[i];
    e1_j = e1[j];
    e0_i = e0[i];
    e0_j = e0[j];

    denom_cross = e0_j * e1_i - e1_j * e0_i;
    if (denom_cross < 0.0f) {
        return 0;
    }

    numerator = e0_i * e2[j] - e0_j * e2[i];
    if (numerator < 0.0f) {
        return 0;
    }

    denom = e2[j] * e1_i - e1_j * e2[i];
    if (numerator + denom_cross <= denom) {
        real inv_denom = 1.0f / denom;
        *out_u = numerator * inv_denom;
        *out_v = inv_denom * denom_cross;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ce8c0):

uint FUN_004ce8c0(float *param_1,float *param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  short sVar4;
  float *in_EAX;
  uint uVar5;
  int iVar6;
  float *in_ECX;
  int iVar7;
  float *in_EDX;
  float *unaff_ESI;
  ushort uVar8;
  float local_40 [4];
  float local_30 [4];
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  local_18 = *unaff_ESI - *in_EAX;
  local_14 = unaff_ESI[1] - in_EAX[1];
  local_10 = unaff_ESI[2] - in_EAX[2];
  local_40[0] = *in_EDX - *in_EAX;
  local_40[1] = in_EDX[1] - in_EAX[1];
  local_40[2] = in_EDX[2] - in_EAX[2];
  local_30[3] = *in_ECX - *in_EAX;
  local_20 = in_ECX[1] - in_EAX[1];
  local_1c = in_ECX[2] - in_EAX[2];
  local_c = local_1c * local_40[1] - local_20 * local_40[2];
  local_30[0] = local_c;
  local_8 = local_40[2] * local_30[3] - local_1c * local_40[0];
  local_30[1] = local_8;
  local_4 = local_20 * local_40[0] - local_40[1] * local_30[3];
  local_30[2] = local_4;
  fVar1 = local_c * (*unaff_ESI - *in_EAX) +
          local_4 * (unaff_ESI[2] - in_EAX[2]) + local_8 * (unaff_ESI[1] - in_EAX[1]);
  fVar1 = fVar1 * fVar1;
  fVar2 = (local_c * local_c + local_8 * local_8 + local_4 * local_4) * 0.0001;
  uVar5 = CONCAT22((short)((uint)local_c >> 0x10),
                   (ushort)(fVar2 < fVar1) << 8 | (ushort)(NAN(fVar2) || NAN(fVar1)) << 10 |
                   (ushort)(fVar2 == fVar1) << 0xe);
  if (fVar2 >= fVar1 && (fVar2 == fVar1) == 0) {
    if ((ABS(local_4) < ABS(local_8)) || (ABS(local_4) < ABS(local_c))) {
      if (ABS(local_8) < ABS(local_c)) {
        sVar4 = 0;
      }
      else {
        sVar4 = 1;
      }
    }
    else {
      sVar4 = 2;
    }
    iVar6 = ((uint)(0.0 < local_30[sVar4]) + sVar4 * 2) * 4;
    iVar7 = (int)*(short *)(&DAT_0065c29c + iVar6);
    iVar6 = (int)*(short *)(&DAT_0065c29e + iVar6);
    local_30[0] = local_40[iVar7];
    local_30[1] = local_40[iVar6];
    local_40[0] = local_30[iVar7 + 6];
    local_40[1] = local_30[iVar6 + 6];
    local_40[3] = local_40[1] * local_30[0] - local_30[1] * local_40[0];
    uVar5 = (uint)(ushort)((ushort)(local_40[3] < 0.0) << 8 | (ushort)NAN(local_40[3]) << 10 |
                          (ushort)(local_40[3] == 0.0) << 0xe);
    if (local_40[3] >= 0.0) {
      fVar1 = local_40[0] * local_30[iVar6 + 3] - local_40[1] * local_30[iVar7 + 3];
      uVar8 = (ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 | (ushort)(fVar1 == 0.0) << 0xe;
      if (fVar1 >= 0.0) {
        fVar2 = local_30[iVar6 + 3] * local_30[0] - local_30[1] * local_30[iVar7 + 3];
        fVar3 = fVar1 + local_40[3];
        uVar8 = (ushort)(fVar3 < fVar2) << 8 | (ushort)(NAN(fVar3) || NAN(fVar2)) << 10 |
                (ushort)(fVar3 == fVar2) << 0xe;
        if (fVar3 < fVar2 != (fVar3 == fVar2)) {
          *param_1 = fVar1 * (1.0 / fVar2);
          *param_2 = (1.0 / fVar2) * local_40[3];
          return CONCAT31((uint3)(byte)(uVar8 >> 8),1);
        }
      }
      uVar5 = (uint)uVar8;
    }
  }
  return uVar5;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
