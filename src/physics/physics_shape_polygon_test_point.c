// physics_shape_polygon_test_point  (Ghidra: FUN_00504120, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x504120, size 318 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: physics_model_shape.plane_i/j/k/d (0x0c..0x18), .thickness (0x1c),
//   .projection_axis/sign (0x20/0x22), .vertex_count (0x24) and .vertices[8][2] (0x28) match
//   in_ECX+0xc.. exactly (out/phase4/physics_types_notes.md).
// register convention: in_ECX -> shape (physics_model_shape *), in_EDX -> point. param_1/param_2
//   are Ghidra-recognized stack parameters (out_depth, out_normal).
//   // blam-cc: ECX -> shape, EDX -> point, stack -> out_depth, out_normal

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, types/math.h (const to
                                                        // match src/math/*.c own declaration)

// blam-cc: ECX -> shape, EDX -> point, stack -> out_depth, out_normal
uint8_t physics_shape_polygon_test_point(physics_model_shape *shape, real_point3d *point,
                                           float *out_depth, real_plane3d *out_normal)
{
    float dist = shape->plane_i * point->x + shape->plane_k * point->z +
                 shape->plane_j * point->y - shape->plane_d;

    if (0.0f <= dist && dist < shape->thickness) {
        float t = -dist;
        real_point3d proj;
        projection_axis_pair proj_axes;
        float proj2d[2];
        int32_t i;

        proj.x = t * shape->plane_i + point->x;
        proj.y = t * shape->plane_j + point->y;
        proj.z = t * shape->plane_k + point->z;
        proj_axes = k_projection_axes[shape->projection_axis * 2 + shape->projection_sign];
        proj2d[0] = ((float *)&proj)[proj_axes.i];
        proj2d[1] = ((float *)&proj)[proj_axes.j];

        for (i = 0; i < shape->vertex_count; i++) {
            int32_t next = (i + 1 < shape->vertex_count) ? (i + 1) : 0;
            float cross = (shape->vertices[next][1] - proj2d[1]) *
                              (shape->vertices[i][0] - proj2d[0]) -
                          (shape->vertices[i][1] - proj2d[1]) *
                              (shape->vertices[next][0] - proj2d[0]);
            if (cross < 0.0f) {
                return 0;
            }
        }

        out_normal->normal.i = shape->plane_i;
        out_normal->normal.j = shape->plane_j;
        out_normal->normal.k = shape->plane_k;
        out_normal->d = shape->plane_d + shape->thickness;
        *out_depth = shape->thickness - dist;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x504120):

uint FUN_00504120(float *param_1,undefined4 *param_2)

{
  float fVar1;
  int iVar2;
  undefined4 uVar3;
  bool bVar4;
  float fVar5;
  int iVar6;
  uint uVar7;
  int in_ECX;
  float *in_EDX;
  uint uVar8;
  float *pfVar9;
  ushort uVar10;
  float local_c [3];

  fVar5 = (*(float *)(in_ECX + 0xc) * *in_EDX +
          *(float *)(in_ECX + 0x14) * in_EDX[2] + *(float *)(in_ECX + 0x10) * in_EDX[1]) -
          *(float *)(in_ECX + 0x18);
  uVar10 = (ushort)(fVar5 < 0.0) << 8 | (ushort)NAN(fVar5) << 10 | (ushort)(fVar5 == 0.0) << 0xe;
  if (fVar5 >= 0.0) {
    fVar1 = *(float *)(in_ECX + 0x1c);
    uVar10 = (ushort)(fVar5 < fVar1) << 8 | (ushort)(NAN(fVar5) || NAN(fVar1)) << 10 |
             (ushort)(fVar5 == fVar1) << 0xe;
    if (fVar5 < fVar1) {
      fVar1 = -fVar5;
      iVar2 = *(int *)(in_ECX + 0x24);
      local_c[0] = fVar1 * *(float *)(in_ECX + 0xc) + *in_EDX;
      local_c[1] = fVar1 * *(float *)(in_ECX + 0x10) + in_EDX[1];
      iVar6 = ((uint)*(byte *)(in_ECX + 0x22) + *(short *)(in_ECX + 0x20) * 2) * 4;
      local_c[2] = fVar1 * *(float *)(in_ECX + 0x14) + in_EDX[2];
      if (0 < iVar2) {
        pfVar9 = (float *)(in_ECX + 0x28);
        uVar8 = 1;
        do {
          uVar7 = (iVar2 <= (int)uVar8) - 1 & uVar8;
          fVar1 = (*(float *)(in_ECX + 0x2c + uVar7 * 8) -
                  local_c[*(short *)(&DAT_0065c29e + iVar6)]) *
                  (*pfVar9 - local_c[*(short *)(&DAT_0065c29c + iVar6)]) -
                  (pfVar9[1] - local_c[*(short *)(&DAT_0065c29e + iVar6)]) *
                  (*(float *)(in_ECX + 0x28 + uVar7 * 8) -
                  local_c[*(short *)(&DAT_0065c29c + iVar6)]);
          if (fVar1 < 0.0) {
            return (uint)(ushort)((ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 |
                                 (ushort)(fVar1 == 0.0) << 0xe);
          }
          pfVar9 = pfVar9 + 2;
          bVar4 = (int)uVar8 < iVar2;
          uVar8 = uVar8 + 1;
        } while (bVar4);
      }
      *param_2 = *(undefined4 *)(in_ECX + 0xc);
      param_2[1] = *(undefined4 *)(in_ECX + 0x10);
      uVar3 = *(undefined4 *)(in_ECX + 0x14);
      param_2[2] = uVar3;
      param_2[3] = *(float *)(in_ECX + 0x18) + *(float *)(in_ECX + 0x1c);
      *param_1 = *(float *)(in_ECX + 0x1c) - fVar5;
      return CONCAT31((int3)((uint)uVar3 >> 8),1);
    }
  }
  return (uint)uVar10;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
