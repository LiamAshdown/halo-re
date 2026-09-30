// decal_build_projection  (Ghidra: FUN_0044e460; named per out/phase4/effects_types_notes.md,
// which refers to this address directly: "decal_build_projection 0x44e460 reads major_axis at
// 0x54, normal_positive at 0x56 and takes 0x58 as the base of the corner array")
// address 0x44e460, size 705 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: types/effects.h decal_projection (every field, sizes match exactly: placement 0x00,
// plane_i..d 0x34, transformed_i..d 0x44, major_axis 0x54, normal_positive 0x56, corners 0x58,
// edge gradients 0x78-0x88, inverse_determinant 0x88); types/math.h projection_axis_pair /
// k_projection_axes (0x0065c29c, already declared, not redefined).
// register convention: the caller-owned 4-float box (param_1) is Ghidra's own recognised stack
// parameter; the placement matrix is in EDX (in_EDX); the output decal_projection* is Ghidra's
// second recognised stack parameter (param_2).
//   // blam-cc: EDX -> placement, stack -> (box, out)
// VERIFIED 2026-09-27 against objdump 0x44e460..0x44e720 (only the |i| == |j| tie-break differed). `box` is a
// {forward_min, forward_max, left_min, left_max} rectangle in the placement's forward/left plane, copied verbatim to
// +0x34; +0x44 is the placement's up vector (the decal plane normal) and its plane distance.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include "fn_effects.h"

extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, math module

// Builds a decal_projection: copies the placement matrix and box verbatim, derives the dominant
// projection axis from the placement's up vector, and flattens the box's four world-space
// corners onto the two remaining axes, along with the edge gradients and inverse determinant
// decal_flood_surfaces and decal_place use to turn a surface point into a uv pair.
void decal_build_projection(real_matrix4x3 *placement, real *box, decal_projection *out)
{
    real corner[3];
    int major_axis;
    uint8_t normal_positive;
    const projection_axis_pair *axes;

    out->placement = *placement;

    out->plane_i = box[0];
    out->plane_j = box[1];
    out->plane_k = box[2];
    out->plane_d = box[3];

    out->transformed_i = placement->up.i;
    out->transformed_j = placement->up.j;
    out->transformed_k = placement->up.k;
    out->transformed_d = out->transformed_i * placement->position.x +
        out->transformed_j * placement->position.y + out->transformed_k * placement->position.z;

    {
        real ai = out->transformed_i < 0.0f ? -out->transformed_i : out->transformed_i;
        real aj = out->transformed_j < 0.0f ? -out->transformed_j : out->transformed_j;
        real ak = out->transformed_k < 0.0f ? -out->transformed_k : out->transformed_k;

        if (ak < aj || ak < ai) {
            major_axis = (aj < ai) ? 0 : 1; // 0x44e4f2: a tie between |i| and |j| picks j
        } else {
            major_axis = 2;
        }
    }
    out->major_axis = (int16_t)major_axis;

    {
        real *component = &out->transformed_i;
        normal_positive = component[major_axis] > 0.0f;
    }
    out->normal_positive = normal_positive;

    axes = &k_projection_axes[major_axis * 2 + normal_positive];

    corner[0] = box[0] * placement->forward.i + box[2] * placement->left.i + placement->position.x;
    corner[1] = box[0] * placement->forward.j + box[2] * placement->left.j + placement->position.y;
    corner[2] = box[0] * placement->forward.k + box[2] * placement->left.k + placement->position.z;
    out->corners[0].u = corner[axes->i];
    out->corners[0].v = corner[axes->j];

    corner[0] = box[1] * placement->forward.i + box[2] * placement->left.i + placement->position.x;
    corner[1] = box[1] * placement->forward.j + box[2] * placement->left.j + placement->position.y;
    corner[2] = box[1] * placement->forward.k + box[2] * placement->left.k + placement->position.z;
    out->corners[1].u = corner[axes->i];
    out->corners[1].v = corner[axes->j];

    corner[0] = box[1] * placement->forward.i + box[3] * placement->left.i + placement->position.x;
    corner[1] = box[1] * placement->forward.j + box[3] * placement->left.j + placement->position.y;
    corner[2] = box[1] * placement->forward.k + box[3] * placement->left.k + placement->position.z;
    out->corners[2].u = corner[axes->i];
    out->corners[2].v = corner[axes->j];

    corner[0] = box[0] * placement->forward.i + box[3] * placement->left.i + placement->position.x;
    corner[1] = box[0] * placement->forward.j + box[3] * placement->left.j + placement->position.y;
    corner[2] = box[0] * placement->forward.k + box[3] * placement->left.k + placement->position.z;
    out->corners[3].u = corner[axes->i];
    out->corners[3].v = corner[axes->j];

    out->du_edge0 = out->corners[1].u - out->corners[0].u;
    out->dv_edge0 = out->corners[1].v - out->corners[0].v;
    out->du_edge1 = out->corners[3].u - out->corners[0].u;
    out->dv_edge1 = out->corners[3].v - out->corners[0].v;
    out->inverse_determinant = 1.0f / (out->dv_edge1 * out->du_edge0 - out->dv_edge0 * out->du_edge1);
}

#if 0
Original Ghidra decompilation (0x44e460):

void FUN_0044e460(float *param_1,undefined4 *param_2)

{
  float *pfVar1;
  float fVar2;
  short sVar3;
  int iVar4;
  undefined4 *in_EDX;
  undefined4 *puVar5;
  undefined4 *puVar6;
  float local_c [3];

  puVar5 = in_EDX;
  puVar6 = param_2;
  for (iVar4 = 0xd; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar6 = puVar6 + 1;
  }
  param_2[0xd] = *param_1;
  param_2[0xe] = param_1[1];
  param_2[0xf] = param_1[2];
  param_2[0x10] = param_1[3];
  pfVar1 = (float *)(param_2 + 0x11);
  *pfVar1 = (float)in_EDX[7];
  param_2[0x12] = in_EDX[8];
  param_2[0x13] = in_EDX[9];
  param_2[0x14] =
       *pfVar1 * (float)in_EDX[10] +
       (float)param_2[0x12] * (float)in_EDX[0xb] + (float)param_2[0x13] * (float)in_EDX[0xc];
  if ((ABS((float)param_2[0x13]) < ABS((float)param_2[0x12])) ||
     (ABS((float)param_2[0x13]) < ABS(*pfVar1))) {
    if (ABS((float)param_2[0x12]) < ABS(*pfVar1)) {
      sVar3 = 0;
    }
    else {
      sVar3 = 1;
    }
  }
  else {
    sVar3 = 2;
  }
  *(short *)(param_2 + 0x15) = sVar3;
  fVar2 = (float)param_2[sVar3 + 0x11];
  *(bool *)((int)param_2 + 0x56) = 0.0 < fVar2;
  iVar4 = ((uint)(0.0 < fVar2) + sVar3 * 2) * 4;
  local_c[0] = param_1[2] * (float)in_EDX[4] + (float)in_EDX[1] * *param_1 + (float)in_EDX[10];
  local_c[1] = (float)in_EDX[2] * *param_1 + (float)in_EDX[5] * param_1[2] + (float)in_EDX[0xb];
  local_c[2] = *param_1 * (float)in_EDX[3] + param_1[2] * (float)in_EDX[6] + (float)in_EDX[0xc];
  fVar2 = local_c[*(short *)(&DAT_0065c29c + iVar4)];
  param_2[0x17] = local_c[*(short *)(&DAT_0065c29e + iVar4)];
  param_2[0x16] = fVar2;
  iVar4 = ((uint)*(byte *)((int)param_2 + 0x56) + *(short *)(param_2 + 0x15) * 2) * 4;
  local_c[0] = param_1[1] * (float)in_EDX[1] + param_1[2] * (float)in_EDX[4] + (float)in_EDX[10];
  local_c[1] = (float)in_EDX[5] * param_1[2] + (float)in_EDX[2] * param_1[1] + (float)in_EDX[0xb];
  local_c[2] = param_1[1] * (float)in_EDX[3] + param_1[2] * (float)in_EDX[6] + (float)in_EDX[0xc];
  fVar2 = local_c[*(short *)(&DAT_0065c29c + iVar4)];
  param_2[0x19] = local_c[*(short *)(&DAT_0065c29e + iVar4)];
  param_2[0x18] = fVar2;
  local_c[0] = (float)in_EDX[4] * param_1[3] + param_1[1] * (float)in_EDX[1] + (float)in_EDX[10];
  local_c[1] = (float)in_EDX[5] * param_1[3] + (float)in_EDX[2] * param_1[1] + (float)in_EDX[0xb];
  local_c[2] = param_1[3] * (float)in_EDX[6] + param_1[1] * (float)in_EDX[3] + (float)in_EDX[0xc];
  iVar4 = ((uint)*(byte *)((int)param_2 + 0x56) + *(short *)(param_2 + 0x15) * 2) * 4;
  sVar3 = *(short *)(&DAT_0065c29c + iVar4);
  param_2[0x1b] = local_c[*(short *)(&DAT_0065c29e + iVar4)];
  param_2[0x1a] = local_c[sVar3];
  local_c[0] = (float)in_EDX[4] * param_1[3] + (float)in_EDX[1] * *param_1 + (float)in_EDX[10];
  local_c[1] = (float)in_EDX[2] * *param_1 + (float)in_EDX[5] * param_1[3] + (float)in_EDX[0xb];
  iVar4 = ((uint)*(byte *)((int)param_2 + 0x56) + *(short *)(param_2 + 0x15) * 2) * 4;
  local_c[2] = *param_1 * (float)in_EDX[3] + param_1[3] * (float)in_EDX[6] + (float)in_EDX[0xc];
  fVar2 = local_c[*(short *)(&DAT_0065c29c + iVar4)];
  param_2[0x1d] = local_c[*(short *)(&DAT_0065c29e + iVar4)];
  param_2[0x1c] = fVar2;
  param_2[0x1e] = (float)param_2[0x18] - (float)param_2[0x16];
  param_2[0x1f] = (float)param_2[0x19] - (float)param_2[0x17];
  param_2[0x20] = (float)param_2[0x1c] - (float)param_2[0x16];
  param_2[0x21] = (float)param_2[0x1d] - (float)param_2[0x17];
  param_2[0x22] =
       1.0 / ((float)param_2[0x21] * (float)param_2[0x1e] -
             (float)param_2[0x1f] * (float)param_2[0x20]);
  return;
}
#endif
