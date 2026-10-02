// physics_shape_surface_to_polygon  (Ghidra: FUN_005038a0, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x5038a0, size 437 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/physics_types_notes.md section 2: "the shape vertex array runs 0x28..
//   0x68, i.e. 8 Point2D, and 0x5038a0 stores vertex_count straight from the surface edge-loop
//   walk without clamping it" -- confirmed here (no clamp against
//   k_maximum_physics_model_shape_vertices); "plane_d pushed out by thickness * plane_k when
//   the source surface faces downward" matches the trailing adjustment block exactly.
// register convention: none -- all eleven arguments are Ghidra-recognized stack parameters.
//   // blam-cc: stack -> vertex_count, vertices, plane, margin, thickness, object_index,
//   //           surface_index, surface_flags, breakable_surface_index, material_type, model

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, types/math.h (const to
                                                        // match src/math/*.c own declaration)
extern double fabs(double x);

// blam-cc: stack -> vertex_count, vertices, plane, margin, thickness, object_index,
//          surface_index, surface_flags, breakable_surface_index, material_type, model
void physics_shape_surface_to_polygon(int16_t vertex_count, real_point3d *vertices,
                                       real_plane3d *plane, float margin, float thickness,
                                       uint32_t object_index, int32_t surface_index,
                                       uint8_t surface_flags, int8_t breakable_surface_index,
                                       int16_t material_type, physics_model *model)
{
    if (model->shape_count < 0x100) {
        physics_model_shape *shape = &model->shapes[model->shape_count];
        model->shape_count += 1;

        shape->object_index = object_index;
        shape->surface_index = surface_index;
        shape->surface_flags = surface_flags;
        shape->breakable_surface_index = breakable_surface_index;
        shape->material_type = material_type;
        shape->plane_i = plane->normal.i;
        shape->plane_j = plane->normal.j;
        shape->plane_k = plane->normal.k;
        shape->plane_d = plane->d;
        shape->thickness = thickness;

        if (((float)fabs((double)shape->plane_k) < (float)fabs((double)shape->plane_j)) ||
            ((float)fabs((double)shape->plane_k) < (float)fabs((double)shape->plane_i))) {
            shape->projection_axis =
                ((float)fabs((double)shape->plane_j) < (float)fabs((double)shape->plane_i)) ? 0
                                                                                              : 1;
        } else {
            shape->projection_axis = 2;
        }
        shape->projection_sign = 0.0f < ((float *)&shape->plane_i)[shape->projection_axis];

        // NOTE: vertex_count is stored as-is, with no clamp against the 8-entry vertex array
        // (out/phase4/physics_types_notes.md), matching the original.
        shape->vertex_count = vertex_count;
        if (0 < vertex_count) {
            projection_axis_pair proj =
                k_projection_axes[shape->projection_axis * 2 + shape->projection_sign];
            int16_t i;
            for (i = 0; i < shape->vertex_count; i++) {
                shape->vertices[i][0] = ((float *)&vertices[i])[proj.i];
                shape->vertices[i][1] = ((float *)&vertices[i])[proj.j];
            }
        }

        if ((0.0f < margin) && (plane->normal.k < 0.0f)) {
            shape->plane_d = shape->plane_d - margin * shape->plane_k;
            if (shape->projection_axis != 2) {
                projection_axis_pair proj =
                    k_projection_axes[shape->projection_axis * 2 + shape->projection_sign];
                int16_t component = (proj.j == 2) ? 1 : 0;
                int16_t i;
                for (i = 0; i < shape->vertex_count; i++) {
                    shape->vertices[i][component] -= margin;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x5038a0):

void FUN_005038a0(short param_1,int param_2,float *param_3,float param_4,undefined4 param_5,
                 undefined4 param_6,undefined4 param_7,undefined1 param_8,undefined1 param_9,
                 undefined2 param_10,int param_11)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 uVar3;
  float fVar4;
  short sVar5;
  int iVar6;
  short sVar7;
  int iVar8;

  sVar5 = *(short *)(param_11 + 4);
  if (sVar5 < 0x100) {
    puVar2 = (undefined4 *)(sVar5 * 0x68 + 0x4408 + param_11);
    *(short *)(param_11 + 4) = sVar5 + 1;
    *puVar2 = param_6;
    puVar2[1] = param_7;
    *(undefined1 *)(puVar2 + 2) = param_8;
    *(undefined1 *)((int)puVar2 + 9) = param_9;
    *(undefined2 *)((int)puVar2 + 10) = param_10;
    puVar2[3] = *param_3;
    puVar2[4] = param_3[1];
    puVar2[5] = param_3[2];
    puVar2[6] = param_3[3];
    puVar2[7] = param_5;
    fVar4 = ABS((float)puVar2[3]);
    if ((ABS((float)puVar2[5]) < ABS((float)puVar2[4])) || (ABS((float)puVar2[5]) < fVar4)) {
      if (ABS((float)puVar2[4]) < fVar4) {
        sVar5 = 0;
      }
      else {
        sVar5 = 1;
      }
    }
    else {
      sVar5 = 2;
    }
    *(short *)(puVar2 + 8) = sVar5;
    *(bool *)((int)puVar2 + 0x22) = 0.0 < (float)puVar2[sVar5 + 3];
    sVar5 = 0;
    puVar2[9] = (int)param_1;
    if (0 < param_1) {
      iVar6 = 0;
      do {
        iVar8 = ((uint)*(byte *)((int)puVar2 + 0x22) + *(short *)(puVar2 + 8) * 2) * 4;
        iVar1 = param_2 + iVar6 * 0xc;
        sVar5 = sVar5 + 1;
        uVar3 = *(undefined4 *)(iVar1 + *(short *)(&DAT_0065c29e + iVar8) * 4);
        puVar2[iVar6 * 2 + 10] = *(undefined4 *)(iVar1 + *(short *)(&DAT_0065c29c + iVar8) * 4);
        puVar2[iVar6 * 2 + 0xb] = uVar3;
        iVar6 = (int)sVar5;
      } while (iVar6 < (int)puVar2[9]);
    }
    if ((0.0 < param_4) && (param_3[2] < 0.0)) {
      puVar2[6] = (float)puVar2[6] - param_4 * (float)puVar2[5];
      if (*(short *)(puVar2 + 8) != 2) {
        sVar5 = *(short *)(&DAT_0065c29e +
                          ((uint)*(byte *)((int)puVar2 + 0x22) + *(short *)(puVar2 + 8) * 2) * 4);
        sVar7 = 0;
        if (0 < (int)puVar2[9]) {
          iVar6 = 0;
          do {
            iVar6 = (short)(ushort)(sVar5 == 2) + 10 + iVar6 * 2;
            sVar7 = sVar7 + 1;
            puVar2[iVar6] = (float)puVar2[iVar6] - param_4;
            iVar6 = (int)sVar7;
          } while (iVar6 < (int)puVar2[9]);
        }
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
