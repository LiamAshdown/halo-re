// model_ik_solve_two_bone  (Ghidra: model_ik_solve_two_bone, already named)
// address 0x4d6440, size 1079 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: out/phase4/models_functions.md summary ("solves a two-bone analytic IK chain,
//   computing the middle joint's position and orientation to reach a target"). VERIFIED
//   against objdump -d -M intel bin/halo.exe (scratchpad/halo_disasm.txt, 0x4d6440..0x4d6825):
//   the four undefined4*/int/float parameters Ghidra shows are all plain real_matrix4x3*
//   stack arguments (no register-passed args at all) -- Ghidra mistypes the second one as
//   float purely because the compiler reused that stack slot's Ghidra-tracked SSA value for
//   an unrelated float local later in the function (`param_2 = fVar5;`); the register EBP
//   that actually holds the pointer is read from [ebp+0x28] as late as 0x4d6751, long after
//   that reassignment, so it never stops being a pointer. This rewrite keeps `middle` as one
//   real_matrix4x3* throughout and gives the float it gets reused for its own name
//   (`middle_target_dist`).
//   The five vector3d_normalize_with_length() calls all drop their ECX argument in Ghidra's
//   decompilation; each is matched to its target by the objdump call site (0x4d657c, 0x4d66a6,
//   0x4d66f9, 0x4d67ab, 0x4d67fe) against the struct field just written immediately before it
//   (a local pole vector, then middle->forward, middle->up, end->forward, end->up in order).
//   Role/anatomy of the four matrices (which is the "shoulder", "elbow" etc.) is not
//   independently confirmed beyond what the read/write pattern below shows -- see the roles
//   note above the function.
// register convention: all four real_matrix4x3* are plain stack parameters, in declaration
//   order.
//   // blam-cc: stack -> target, middle, end, out_end

#include "tags.h"
#include "math.h"
#include "fn_math.h"

extern double sqrt(double x); // a single x87 FSQRT instruction, see src/math/quaternion_normalize.c


// Roles (review pass, from the reads and writes): `middle` is the ROOT joint of the chain
// (shoulder / hip): its position is only read, its rotation is re-aimed. `end` is the MIDDLE
// joint (elbow / knee): its old position gives the upper bone length, its rotation is re-aimed
// and its position is moved to root + upper_length * root.forward. `out_end` is the EFFECTOR
// (hand / foot): its old position gives the lower bone length, then it receives a copy of
// `target`. `target` is the goal; its position is pulled in to 0.98 of full reach if needed.
// Both rebuilt bases are forward (solved), up = forward x old left, left = up x forward.
// Review fix: the first rewrite computed left as forward x up (the negation) for both joints.
//
// middle's orientation is solved to face the point (`projected` back from `target` along the
// pivot axis, offset `perp` to the side) that keeps it bone_end_middle away from where `end`
// ends up; end's orientation faces the complementary offset, back towards target.
void model_ik_solve_two_bone(real_matrix4x3 *target, real_matrix4x3 *middle, real_matrix4x3 *end,
                              real_matrix4x3 *out_end)
{
    real bone_end_middle;   // |end->position - middle->position|
    real reach_beyond_end;  // |out_end->position - end->position| (out_end's ORIGINAL position)
    real middle_target_dist; // |target->position - middle->position|, clamped for max reach
    real dir_x, dir_y, dir_z; // unit vector from middle->position to target->position
    real pole_x, pole_y, pole_z; // unit normal of the middle/end/target plane
    real max_reach;
    real projected, remaining, perp;

    {
        real dx = end->position.x - middle->position.x;
        real dy = end->position.y - middle->position.y;
        real dz = end->position.z - middle->position.z;
        bone_end_middle = (real)sqrt((double)(dx * dx + dz * dz + dy * dy));
    }
    {
        real dx = out_end->position.x - end->position.x;
        real dy = out_end->position.y - end->position.y;
        real dz = out_end->position.z - end->position.z;
        reach_beyond_end = (real)sqrt((double)(dx * dx + dy * dy + dz * dz));
    }
    {
        real dx = middle->position.x - target->position.x;
        real dy = middle->position.y - target->position.y;
        real dz = middle->position.z - target->position.z;
        middle_target_dist = (real)sqrt((double)(dx * dx + dy * dy + dz * dz));
    }

    {
        real inv = 1.0f / middle_target_dist;
        real ex = end->position.x - middle->position.x;
        real ey = end->position.y - middle->position.y;
        real ez = end->position.z - middle->position.z;
        real_vector3d pole;

        dir_x = (target->position.x - middle->position.x) * inv;
        dir_y = (target->position.y - middle->position.y) * inv;
        dir_z = (target->position.z - middle->position.z) * inv;

        pole.i = dir_y * ez - dir_z * ey;
        pole.j = dir_z * ex - dir_x * ez;
        pole.k = dir_x * ey - dir_y * ex;
        vector3d_normalize_with_length(&pole); // 0x4d657c
        pole_x = pole.i;
        pole_y = pole.j;
        pole_z = pole.k;
    }

    max_reach = (reach_beyond_end + bone_end_middle) * 0.98f;
    if (max_reach < middle_target_dist) {
        target->position.x = dir_x * max_reach + middle->position.x;
        target->position.y = dir_y * max_reach + middle->position.y;
        target->position.z = dir_z * max_reach + middle->position.z;
        middle_target_dist = max_reach;
    }

    // Law-of-cosines split of the middle/target axis: `projected` is how far along that axis
    // the bone_end_middle/reach_beyond_end joint falls, `perp` is the perpendicular offset.
    projected = ((middle_target_dist * middle_target_dist + bone_end_middle * bone_end_middle) -
                 reach_beyond_end * reach_beyond_end) / (middle_target_dist + middle_target_dist);
    remaining = middle_target_dist - projected;
    perp = (real)sqrt((double)(bone_end_middle * bone_end_middle - projected * projected));

    {
        real side_x = pole_y * dir_z - pole_z * dir_y;
        real side_y = pole_z * dir_x - pole_x * dir_z;
        real side_z = pole_x * dir_y - pole_y * dir_x;

        middle->forward.i = projected * dir_x + side_x * perp;
        middle->forward.j = projected * dir_y + side_y * perp;
        middle->forward.k = projected * dir_z + side_z * perp;
        vector3d_normalize_with_length(&middle->forward); // 0x4d66a6

        // Rebuild an orthonormal basis from the new forward, seeded by the old left.
        middle->up.i = middle->left.k * middle->forward.j - middle->left.j * middle->forward.k;
        middle->up.j = middle->left.i * middle->forward.k - middle->left.k * middle->forward.i;
        middle->up.k = middle->forward.i * middle->left.j - middle->left.i * middle->forward.j;
        vector3d_normalize_with_length(&middle->up); // 0x4d66f9

        // left = up x forward (0x4d6700..0x4d674c; every fsubp here is DE E9, st1 - st0)
        middle->left.i = middle->up.j * middle->forward.k - middle->up.k * middle->forward.j;
        middle->left.j = middle->up.k * middle->forward.i - middle->up.i * middle->forward.k;
        middle->left.k = middle->up.i * middle->forward.j - middle->up.j * middle->forward.i;

        end->forward.i = remaining * dir_x - side_x * perp;
        end->forward.j = remaining * dir_y - side_y * perp;
        end->forward.k = remaining * dir_z - side_z * perp;
        vector3d_normalize_with_length(&end->forward); // 0x4d67ab

        end->up.i = end->left.k * end->forward.j - end->left.j * end->forward.k;
        end->up.j = end->left.i * end->forward.k - end->left.k * end->forward.i;
        end->up.k = end->forward.i * end->left.j - end->left.i * end->forward.j;
        vector3d_normalize_with_length(&end->up); // 0x4d67fe

        // left = up x forward (0x4d6805..0x4d6855)
        end->left.i = end->up.j * end->forward.k - end->up.k * end->forward.j;
        end->left.j = end->up.k * end->forward.i - end->up.i * end->forward.k;
        end->left.k = end->up.i * end->forward.j - end->up.j * end->forward.i;

        end->position.x = bone_end_middle * middle->forward.i + middle->position.x;
        end->position.y = bone_end_middle * middle->forward.j + middle->position.y;
        end->position.z = bone_end_middle * middle->forward.k + middle->position.z;
    }

    *out_end = *target;
}

#if 0
Original Ghidra decompilation (0x4d6440):

void model_ik_solve_two_bone(undefined4 *param_1,float param_2,int param_3,undefined4 *param_4)

{
  float *pfVar1;
  float *pfVar2;
  float *pfVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  int iVar17;
  float *pfVar18;

  iVar17 = (int)param_2;
  fVar10 = *(float *)(param_3 + 0x28) - *(float *)((int)param_2 + 0x28);
  fVar4 = *(float *)(param_3 + 0x2c) - *(float *)((int)param_2 + 0x2c);
  pfVar1 = (float *)(param_3 + 0x28);
  fVar5 = *(float *)(param_3 + 0x30) - *(float *)((int)param_2 + 0x30);
  fVar10 = SQRT(fVar10 * fVar10 + fVar5 * fVar5 + fVar4 * fVar4);
  fVar5 = (float)param_4[0xb] - *(float *)(param_3 + 0x2c);
  fVar4 = (float)param_4[0xc] - *(float *)(param_3 + 0x30);
  fVar4 = SQRT(((float)param_4[10] - *pfVar1) * ((float)param_4[10] - *pfVar1) +
               fVar5 * fVar5 + fVar4 * fVar4);
  fVar5 = *(float *)((int)param_2 + 0x28) - (float)param_1[10];
  fVar7 = *(float *)((int)param_2 + 0x2c) - (float)param_1[0xb];
  fVar6 = *(float *)((int)param_2 + 0x30) - (float)param_1[0xc];
  fVar5 = SQRT(fVar5 * fVar5 + fVar7 * fVar7 + fVar6 * fVar6);
  fVar6 = *pfVar1 - *(float *)((int)param_2 + 0x28);
  fVar9 = *(float *)(param_3 + 0x2c) - *(float *)((int)param_2 + 0x2c);
  fVar8 = *(float *)(param_3 + 0x30) - *(float *)((int)param_2 + 0x30);
  fVar11 = 1.0 / fVar5;
  fVar12 = ((float)param_1[10] - *(float *)((int)param_2 + 0x28)) * fVar11;
  fVar13 = ((float)param_1[0xb] - *(float *)((int)param_2 + 0x2c)) * fVar11;
  fVar11 = ((float)param_1[0xc] - *(float *)((int)param_2 + 0x30)) * fVar11;
  fVar7 = fVar13 * fVar8 - fVar11 * fVar9;
  fVar8 = fVar11 * fVar6 - fVar12 * fVar8;
  fVar6 = fVar12 * fVar9 - fVar13 * fVar6;
  vector3d_normalize_with_length();
  fVar9 = (fVar4 + fVar10) * 0.98;
  if (fVar9 < fVar5) {
    param_1[10] = fVar12 * fVar9 + *(float *)((int)param_2 + 0x28);
    param_1[0xb] = fVar13 * fVar9 + *(float *)((int)param_2 + 0x2c);
    param_1[0xc] = fVar11 * fVar9 + *(float *)((int)param_2 + 0x30);
    fVar5 = fVar9;
  }
  param_2 = fVar5;
  pfVar2 = (float *)(iVar17 + 4);
  pfVar3 = (float *)(iVar17 + 0x1c);
  fVar4 = ((param_2 * param_2 + fVar10 * fVar10) - fVar4 * fVar4) / (param_2 + param_2);
  param_2 = param_2 - fVar4;
  fVar14 = SQRT(fVar10 * fVar10 - fVar4 * fVar4);
  fVar15 = (fVar8 * fVar11 - fVar6 * fVar13) * fVar14;
  *pfVar2 = fVar4 * fVar12 + fVar15;
  fVar16 = (fVar6 * fVar12 - fVar7 * fVar11) * fVar14;
  *(float *)(iVar17 + 8) = fVar4 * fVar13 + fVar16;
  fVar14 = (fVar7 * fVar13 - fVar8 * fVar12) * fVar14;
  *(float *)(iVar17 + 0xc) = fVar4 * fVar11 + fVar14;
  vector3d_normalize_with_length();
  *pfVar3 = *(float *)(iVar17 + 0x18) * *(float *)(iVar17 + 8) -
            *(float *)(iVar17 + 0xc) * *(float *)(iVar17 + 0x14);
  *(float *)(iVar17 + 0x20) =
       *(float *)(iVar17 + 0x10) * *(float *)(iVar17 + 0xc) - *(float *)(iVar17 + 0x18) * *pfVar2;
  *(float *)(iVar17 + 0x24) =
       *pfVar2 * *(float *)(iVar17 + 0x14) - *(float *)(iVar17 + 0x10) * *(float *)(iVar17 + 8);
  vector3d_normalize_with_length();
  *(float *)(iVar17 + 0x10) =
       *(float *)(iVar17 + 0xc) * *(float *)(iVar17 + 0x20) -
       *(float *)(iVar17 + 0x24) * *(float *)(iVar17 + 8);
  *(float *)(iVar17 + 0x14) =
       *pfVar2 * *(float *)(iVar17 + 0x24) - *pfVar3 * *(float *)(iVar17 + 0xc);
  *(float *)(iVar17 + 0x18) = *pfVar3 * *(float *)(iVar17 + 8) - *pfVar2 * *(float *)(iVar17 + 0x20)
  ;
  fVar4 = *pfVar2;
  fVar5 = *(float *)(iVar17 + 0x28);
  fVar6 = *(float *)(iVar17 + 8);
  fVar7 = *(float *)(iVar17 + 0x2c);
  fVar8 = *(float *)(iVar17 + 0xc);
  pfVar2 = (float *)(param_3 + 4);
  fVar9 = *(float *)(iVar17 + 0x30);
  pfVar3 = (float *)(param_3 + 0x10);
  pfVar18 = (float *)(param_3 + 0x1c);
  *pfVar2 = param_2 * fVar12 - fVar15;
  *(float *)(param_3 + 8) = param_2 * fVar13 - fVar16;
  *(float *)(param_3 + 0xc) = param_2 * fVar11 - fVar14;
  vector3d_normalize_with_length();
  *pfVar18 = *(float *)(param_3 + 0x18) * *(float *)(param_3 + 8) -
             *(float *)(param_3 + 0x14) * *(float *)(param_3 + 0xc);
  *(float *)(param_3 + 0x20) =
       *pfVar3 * *(float *)(param_3 + 0xc) - *(float *)(param_3 + 0x18) * *pfVar2;
  *(float *)(param_3 + 0x24) =
       *(float *)(param_3 + 0x14) * *pfVar2 - *pfVar3 * *(float *)(param_3 + 8);
  vector3d_normalize_with_length();
  *pfVar3 = *(float *)(param_3 + 0x20) * *(float *)(param_3 + 0xc) -
            *(float *)(param_3 + 0x24) * *(float *)(param_3 + 8);
  *(float *)(param_3 + 0x14) =
       *(float *)(param_3 + 0x24) * *pfVar2 - *pfVar18 * *(float *)(param_3 + 0xc);
  *(float *)(param_3 + 0x18) =
       *pfVar18 * *(float *)(param_3 + 8) - *(float *)(param_3 + 0x20) * *pfVar2;
  *pfVar1 = fVar10 * fVar4 + fVar5;
  *(float *)(param_3 + 0x2c) = fVar10 * fVar6 + fVar7;
  *(float *)(param_3 + 0x30) = fVar10 * fVar8 + fVar9;
  for (iVar17 = 0xd; iVar17 != 0; iVar17 = iVar17 + -1) {
    *param_4 = *param_1;
    param_1 = param_1 + 1;
    param_4 = param_4 + 1;
  }
  return;
}
#endif
