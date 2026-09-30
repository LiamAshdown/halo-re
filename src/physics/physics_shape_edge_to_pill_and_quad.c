// physics_shape_edge_to_pill_and_quad  (Ghidra: FUN_00503490, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503490, size 1028 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/physics_types_notes.md section 2 derives physics_model_pill and
//   physics_model_shape's layout from this function's stores directly, mirroring
//   physics_shape_vertex_to_sphere's height_offset pattern one level up (an edge instead of a
//   vertex). Builds two pills (top and bottom-offset capsule caps) plus two opposing flat quad
//   shapes for the vertical "wall" along the edge, one facing each way.
// register convention: in_EAX -> model (physics_model *), in_ECX -> near_vertex (real_point3d *),
//   in_EDX -> edge_dir (real_vector3d *). param_1..param_7 are Ghidra-recognized stack
//   parameters.
//   // blam-cc: EAX -> model, ECX -> near_vertex, EDX -> edge_dir,
//   //           stack -> height_offset, thickness, object_index, surface_index, surface_flags,
//   //           breakable_surface_index, material_type
// UNSURE: the two calls to vector3d_major_axis_index() show no visible argument; reconstructed
// as the address of the plane normal this function just stored (&shape->plane_i), which is
// exactly the dominant-axis-of-a-plane-normal idiom used everywhere else in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#include "fn_math.h"
#include "fn_physics.h"

extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, types/math.h (const to
                                                        // match src/math/*.c own declaration)
extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction
extern double fabs(double x);


static void append_quad_vertices(physics_model_shape *shape, float quad[4][3],
                                  projection_axis_pair proj)
{
    int i;
    for (i = 0; i < 4; i++) {
        shape->vertices[i][0] = quad[i][proj.i];
        shape->vertices[i][1] = quad[i][proj.j];
    }
}

// blam-cc: EAX -> model, ECX -> near_vertex, EDX -> edge_dir,
//          stack -> height_offset, thickness, object_index, surface_index, surface_flags,
//          breakable_surface_index, material_type
void physics_shape_edge_to_pill_and_quad(physics_model *model, real_point3d *near_vertex,
                                          real_vector3d *edge_dir, float height_offset,
                                          float thickness, uint32_t object_index,
                                          int32_t surface_index, uint8_t surface_flags,
                                          int8_t breakable_surface_index, int16_t material_type)
{
    if (model->pill_count < 0x100) {
        physics_model_pill *pill = &model->pills[model->pill_count];
        model->pill_count += 1;
        pill->object_index = object_index;
        pill->surface_index = surface_index;
        pill->surface_flags = surface_flags;
        pill->breakable_surface_index = breakable_surface_index;
        pill->material_type = material_type;
        pill->origin_x = near_vertex->x;
        pill->origin_y = near_vertex->y;
        pill->origin_z = near_vertex->z;
        pill->extent_i = edge_dir->i;
        pill->extent_j = edge_dir->j;
        pill->extent_k = edge_dir->k;
        pill->radius = thickness;
    }

    if (0.0f < height_offset) {
        if (model->pill_count < 0x100) {
            physics_model_pill *pill = &model->pills[model->pill_count];
            model->pill_count += 1;
            pill->object_index = object_index;
            pill->surface_index = surface_index;
            pill->surface_flags = surface_flags;
            pill->breakable_surface_index = breakable_surface_index;
            pill->material_type = material_type;
            pill->origin_x = near_vertex->x;
            pill->origin_y = near_vertex->y;
            pill->origin_z = near_vertex->z - height_offset;
            pill->extent_i = edge_dir->i;
            pill->extent_j = edge_dir->j;
            pill->extent_k = edge_dir->k;
            pill->radius = thickness;
        }

        {
            float perp_i = -edge_dir->j;
            float perp_j = edge_dir->i;
            float perp_len = (float)sqrt((double)(edge_dir->i * edge_dir->i + perp_i * perp_i));

            if (0.0001 <= (float)fabs((double)perp_len)) {
                perp_i = (1.0f / perp_len) * perp_i;
                perp_j = (1.0f / perp_len) * perp_j;
                if (perp_len != 0.0f) {
                    // the four corners of the vertical "wall" quad along this edge: near-top,
                    // far-top, far-bottom, near-bottom
                    float quad[4][3];
                    float plane_d;

                    quad[0][0] = near_vertex->x;
                    quad[0][1] = near_vertex->y;
                    quad[0][2] = near_vertex->z;
                    plane_d = perp_i * near_vertex->x + perp_j * near_vertex->y;
                    quad[1][0] = near_vertex->x + edge_dir->i;
                    quad[1][1] = near_vertex->y + edge_dir->j;
                    quad[1][2] = near_vertex->z + edge_dir->k;
                    quad[3][0] = near_vertex->x;
                    quad[3][1] = near_vertex->y;
                    quad[3][2] = near_vertex->z - height_offset;
                    quad[2][0] = quad[1][0];
                    quad[2][1] = quad[1][1];
                    quad[2][2] = quad[1][2] - height_offset;

                    if (model->shape_count < 0x100) {
                        physics_model_shape *shape = &model->shapes[model->shape_count];
                        model->shape_count += 1;
                        shape->object_index = object_index;
                        shape->surface_index = surface_index;
                        shape->surface_flags = surface_flags;
                        shape->breakable_surface_index = breakable_surface_index;
                        shape->material_type = material_type;
                        shape->plane_i = perp_i;
                        shape->plane_j = perp_j;
                        shape->plane_k = 0.0f;
                        shape->plane_d = plane_d;
                        shape->thickness = thickness;
                        shape->projection_axis = vector3d_major_axis_index(
                            (real_vector3d *)&shape->plane_i);
                        shape->projection_sign =
                            0.0f < ((float *)&shape->plane_i)[shape->projection_axis];
                        shape->vertex_count = 4;
                        {
                            projection_axis_pair proj = k_projection_axes[shape->projection_axis * 2 +
                                                                           shape->projection_sign];
                            append_quad_vertices(shape, quad, proj);
                        }
                    }

                    // reorder to near-top, near-bottom, far-bottom, far-top for the opposite
                    // face, keeping the same four physical corners
                    {
                        float new_quad[4][3];
                        new_quad[0][0] = quad[0][0];
                        new_quad[0][1] = quad[0][1];
                        new_quad[0][2] = quad[0][2];
                        new_quad[1][0] = quad[3][0];
                        new_quad[1][1] = quad[3][1];
                        new_quad[1][2] = quad[3][2];
                        new_quad[2][0] = quad[2][0];
                        new_quad[2][1] = quad[2][1];
                        new_quad[2][2] = quad[2][2];
                        new_quad[3][0] = quad[1][0];
                        new_quad[3][1] = quad[1][1];
                        new_quad[3][2] = quad[1][2];

                        if (model->shape_count < 0x100) {
                            physics_model_shape *shape = &model->shapes[model->shape_count];
                            model->shape_count += 1;
                            shape->object_index = object_index;
                            shape->surface_index = surface_index;
                            shape->surface_flags = surface_flags;
                            shape->breakable_surface_index = breakable_surface_index;
                            shape->material_type = material_type;
                            shape->plane_i = -perp_i;
                            shape->plane_j = -perp_j;
                            shape->plane_k = 0.0f;
                            shape->plane_d = -plane_d;
                            shape->thickness = thickness;
                            shape->projection_axis = vector3d_major_axis_index(
                                (real_vector3d *)&shape->plane_i);
                            shape->projection_sign =
                                0.0f < ((float *)&shape->plane_i)[shape->projection_axis];
                            shape->vertex_count = 4;
                            {
                                projection_axis_pair proj =
                                    k_projection_axes[shape->projection_axis * 2 +
                                                       shape->projection_sign];
                                append_quad_vertices(shape, new_quad, proj);
                            }
                        }
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x503490):

void FUN_00503490(float param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined1 param_5,undefined1 param_6,undefined2 param_7)

{
  undefined4 *puVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  short sVar8;
  int in_EAX;
  int iVar9;
  float *in_ECX;
  int iVar10;
  float *in_EDX;
  short sVar11;
  float afStack_80030 [131065];
  float local_30 [4];
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  sVar8 = *(short *)(in_EAX + 2);
  if (sVar8 < 0x100) {
    iVar9 = (int)sVar8;
    *(short *)(in_EAX + 2) = sVar8 + 1;
    *(undefined4 *)(in_EAX + 0x1c08 + iVar9 * 0x28) = param_3;
    *(undefined4 *)(in_EAX + 0x1c0c + iVar9 * 0x28) = param_4;
    iVar9 = in_EAX + 0x1c08 + iVar9 * 0x28;
    *(undefined1 *)(iVar9 + 8) = param_5;
    *(undefined1 *)(iVar9 + 9) = param_6;
    *(undefined2 *)(iVar9 + 10) = param_7;
    *(float *)(iVar9 + 0xc) = *in_ECX;
    *(float *)(iVar9 + 0x10) = in_ECX[1];
    *(float *)(iVar9 + 0x14) = in_ECX[2];
    *(float *)(iVar9 + 0x18) = *in_EDX;
    *(float *)(iVar9 + 0x1c) = in_EDX[1];
    *(float *)(iVar9 + 0x20) = in_EDX[2];
    *(undefined4 *)(iVar9 + 0x24) = param_2;
  }
  if (0.0 < param_1) {
    sVar8 = *(short *)(in_EAX + 2);
    if (sVar8 < 0x100) {
      iVar9 = (int)sVar8;
      *(short *)(in_EAX + 2) = sVar8 + 1;
      *(undefined4 *)(in_EAX + 0x1c08 + iVar9 * 0x28) = param_3;
      *(undefined4 *)(in_EAX + 0x1c0c + iVar9 * 0x28) = param_4;
      iVar9 = in_EAX + 0x1c08 + iVar9 * 0x28;
      *(undefined1 *)(iVar9 + 8) = param_5;
      *(undefined1 *)(iVar9 + 9) = param_6;
      *(undefined2 *)(iVar9 + 10) = param_7;
      fVar2 = in_ECX[2];
      fVar3 = in_ECX[1];
      *(float *)(iVar9 + 0xc) = *in_ECX;
      *(float *)(iVar9 + 0x10) = fVar3;
      *(float *)(iVar9 + 0x14) = fVar2 - param_1;
      *(float *)(iVar9 + 0x18) = *in_EDX;
      *(float *)(iVar9 + 0x1c) = in_EDX[1];
      *(float *)(iVar9 + 0x20) = in_EDX[2];
      *(undefined4 *)(iVar9 + 0x24) = param_2;
    }
    fVar3 = -in_EDX[1];
    fVar2 = *in_EDX;
    fVar5 = SQRT(fVar2 * fVar2 + fVar3 * fVar3);
    if (0.0001 <= ABS(fVar5)) {
      fVar3 = (1.0 / fVar5) * fVar3;
      fVar2 = (1.0 / fVar5) * fVar2;
      if (fVar5 != 0.0) {
        local_30[0] = *in_ECX;
        local_30[1] = in_ECX[1];
        local_30[2] = in_ECX[2];
        fVar5 = fVar3 * *in_ECX + fVar2 * in_ECX[1];
        sVar11 = 0;
        local_30[3] = *in_ECX + *in_EDX;
        local_20 = in_ECX[1] + in_EDX[1];
        local_1c = in_ECX[2] + in_EDX[2];
        local_c = *in_ECX;
        local_10 = local_1c - param_1;
        local_8 = in_ECX[1];
        sVar8 = *(short *)(in_EAX + 4);
        local_4 = in_ECX[2] - param_1;
        local_18 = local_30[3];
        local_14 = local_20;
        if (sVar8 < 0x100) {
          puVar1 = (undefined4 *)(sVar8 * 0x68 + 0x4408 + in_EAX);
          *(short *)(in_EAX + 4) = sVar8 + 1;
          *puVar1 = param_3;
          puVar1[1] = param_4;
          *(undefined1 *)(puVar1 + 2) = param_5;
          *(undefined1 *)((int)puVar1 + 9) = param_6;
          *(undefined2 *)((int)puVar1 + 10) = param_7;
          puVar1[3] = fVar3;
          puVar1[4] = fVar2;
          puVar1[5] = 0;
          puVar1[6] = fVar5;
          puVar1[7] = param_2;
          sVar8 = vector3d_major_axis_index();
          *(short *)(puVar1 + 8) = sVar8;
          *(bool *)((int)puVar1 + 0x22) = 0.0 < (float)puVar1[sVar8 + 3];
          puVar1[9] = 4;
          iVar9 = 0;
          do {
            iVar10 = ((uint)*(byte *)((int)puVar1 + 0x22) + *(short *)(puVar1 + 8) * 2) * 4;
            sVar11 = sVar11 + 1;
            fVar4 = local_30[iVar9 * 3 + (int)*(short *)(&DAT_0065c29c + iVar10)];
            puVar1[iVar9 * 2 + 0xb] = local_30[iVar9 * 3 + (int)*(short *)(&DAT_0065c29e + iVar10)];
            puVar1[iVar9 * 2 + 10] = fVar4;
            iVar9 = (int)sVar11;
          } while (iVar9 < (int)puVar1[9]);
        }
        fVar7 = local_1c;
        fVar6 = local_20;
        fVar4 = local_30[3];
        local_30[3] = local_c;
        local_c = fVar4;
        sVar8 = *(short *)(in_EAX + 4);
        local_20 = local_8;
        local_1c = local_4;
        local_8 = fVar6;
        local_4 = fVar7;
        if (sVar8 < 0x100) {
          puVar1 = (undefined4 *)(sVar8 * 0x68 + 0x4408 + in_EAX);
          *(short *)(in_EAX + 4) = sVar8 + 1;
          puVar1[1] = param_4;
          *puVar1 = param_3;
          *(undefined2 *)((int)puVar1 + 10) = param_7;
          *(undefined1 *)(puVar1 + 2) = param_5;
          *(undefined1 *)((int)puVar1 + 9) = param_6;
          sVar11 = 0;
          puVar1[3] = -fVar3;
          puVar1[5] = 0;
          puVar1[4] = -fVar2;
          puVar1[7] = param_2;
          puVar1[6] = -fVar5;
          sVar8 = vector3d_major_axis_index();
          *(short *)(puVar1 + 8) = sVar8;
          *(bool *)((int)puVar1 + 0x22) = 0.0 < (float)puVar1[sVar8 + 3];
          puVar1[9] = 4;
          iVar9 = 0;
          do {
            iVar10 = ((uint)*(byte *)((int)puVar1 + 0x22) + *(short *)(puVar1 + 8) * 2) * 4;
            sVar11 = sVar11 + 1;
            fVar2 = local_30[iVar9 * 3 + (int)*(short *)(&DAT_0065c29c + iVar10)];
            puVar1[iVar9 * 2 + 0xb] = local_30[iVar9 * 3 + (int)*(short *)(&DAT_0065c29e + iVar10)];
            puVar1[iVar9 * 2 + 10] = fVar2;
            iVar9 = (int)sVar11;
          } while (iVar9 < (int)puVar1[9]);
          return;
        }
      }
    }
  }
  return;
}
#endif
