// actor_movement_collect_obstacle_candidates  (Ghidra: actor_movement_collect_obstacle_candidates, already named)
// address 0x418ce0, size 656 bytes
// name confidence: 0.6   rewrite confidence: 0.65
// evidence: fills actor_movement_context.obstacles from an object_find_in_sphere sweep
// around the actor's own unit. For every candidate that is not the actor's own unit and
// whose collision model has at least one pathfinding sphere, it computes the largest
// horizontal distance from the object origin to any pathfinding sphere (sphere centre
// transformed by the owning node matrix, plus the node-scaled sphere radius) and records
// {object, x, y, bottom, height, radius} in the obstacle array, refusing to grow past 1024.
// register convention: the context pointer is a genuine stack parameter.
// blam-cc: stack -> context
//
// Facts recovered from the disassembly that Ghidra hides:
//  - object_get_world_matrix writes into [esp+0x40], the same 0x34-byte slot the sphere loop
//    passes as the transform matrix when the sphere is not attached to a node; Ghidra calls
//    that slot local_2040 and prints only its first float, which is real_matrix4x3.scale.
//  - matrix4x3_transform_point takes the output point in EAX and the source point in EDX
//    (record + 0x10 == ModelCollisionGeometrySphere.center) with the matrix pushed; Ghidra
//    prints only the pushed matrix.
// UNSURE: the two register arguments of object_get_world_matrix and the trailing literal 0 it
// is passed are read off this call site only; that function is not rewritten.
// UNSURE: object + 0x1f2 is an int16 byte offset from the object base to its node matrix
// array (stride 0x34); types/objects.h does not name it.
// UNSURE: "bottom" and "height" are the names types/ai.h already gives the two derived
// floats; this function is the only writer and neither name is independently confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

extern int16_t object_find_in_sphere(int32_t kind, int32_t type_mask, const void *from,
                                     const real_point3d *center, float radius,
                                     datum_index *out_objects, int32_t maximum_count); // 0x4f6fe0
extern real_matrix4x3 *object_get_world_matrix(uint32_t object_index, real_matrix4x3 *out); // 0x4f6a20,
                                               // blam-cc: EAX -> object_index, EDI -> out (matches src/objects)
extern void matrix4x3_transform_point(real_point3d *out, const real_point3d *point,
                                      const real_matrix4x3 *matrix); // 0x4cbde0, EAX/EDX + stack

// blam-cc: stack -> context
void actor_movement_collect_obstacle_candidates(actor_movement_context *context)
{
    datum_index candidates[2048];
    real_matrix4x3 world_matrix;
    real_point3d transformed;
    object *unit_object;
    object *candidate_object;
    Object *candidate_definition;
    ModelCollisionGeometry *collision_model;
    ModelCollisionGeometrySphere *spheres;
    ModelCollisionGeometrySphere *sphere;
    const real_matrix4x3 *node_matrix;
    float origin_x, origin_y, origin_z, origin_top;
    float extent;
    float reach;
    float dx, dy;
    int16_t found;
    int16_t i;
    int16_t obstacle_index;
    int32_t sphere_count;
    int32_t remaining;
    datum_index *cursor;

    unit_object = ((object_header *)object_data->data)[context->unit_index & 0xffff].data;
    found = object_find_in_sphere(1, 0xc2, (uint8_t *)unit_object + 0x98, &context->position,
                                  context->search_radius, candidates, 0x800);
    context->obstacle_count = 0;
    if (found <= 0) {
        return;
    }

    cursor = candidates;
    remaining = (int32_t)(uint16_t)found;
    do {
        if (*cursor != (datum_index)k_datum_index_none && *cursor != context->unit_index) {
            candidate_object = ((object_header *)object_data->data)[*cursor & 0xffff].data;
            candidate_definition = (Object *)tag_instances[candidate_object->definition_tag & 0xffff].data;
            collision_model = (ModelCollisionGeometry *)
                tag_instances[candidate_definition->collision_model.tag_id.index].data;
            if ((int32_t)collision_model->pathfinding_spheres.count > 0) {
                candidate_object = ((object_header *)object_data->data)[*cursor & 0xffff].data;
                origin_x = *(float *)((uint8_t *)candidate_object + 0xa0);
                origin_y = *(float *)((uint8_t *)candidate_object + 0xa4);
                origin_z = *(float *)((uint8_t *)candidate_object + 0xa8);
                origin_top = *(float *)((uint8_t *)candidate_object + 0xac);
                extent = 0.0f;
                // Two register arguments only; the definition in src/objects takes the object
                // INDEX in EAX, so the cursor handle is passed rather than the object pointer.
                object_get_world_matrix(*cursor, &world_matrix);

                sphere_count = (int32_t)collision_model->pathfinding_spheres.count;
                spheres = (ModelCollisionGeometrySphere *)collision_model->pathfinding_spheres.pointer;
                for (i = 0; (int32_t)i < sphere_count; i++) {
                    sphere = &spheres[i];
                    if ((int16_t)sphere->node == -1) {
                        matrix4x3_transform_point(&transformed, (const real_point3d *)&sphere->center,
                                                  &world_matrix);
                        reach = world_matrix.scale * sphere->radius;
                    } else {
                        candidate_object = ((object_header *)object_data->data)[*cursor & 0xffff].data;
                        node_matrix = (const real_matrix4x3 *)
                            ((uint8_t *)candidate_object +
                             (int32_t)*(int16_t *)((uint8_t *)candidate_object + 0x1f2) +
                             (int16_t)sphere->node * 0x34);
                        matrix4x3_transform_point(&transformed, (const real_point3d *)&sphere->center,
                                                  node_matrix);
                        reach = sphere->radius * node_matrix->scale;
                    }
                    dx = transformed.x - origin_x;
                    dy = transformed.y - origin_y;
                    reach = (float)sqrt((double)(dy * dy + dx * dx)) + reach;
                    if (extent <= reach) {
                        extent = reach;
                    }
                }

                obstacle_index = context->obstacle_count;
                if (obstacle_index < 0x400) {
                    actor_movement_obstacle *obstacle = &context->obstacles[obstacle_index];

                    context->obstacle_count = (int16_t)(obstacle_index + 1);
                    obstacle->object_index = *cursor;
                    obstacle->radius = extent;
                    obstacle->position.x = origin_x;
                    obstacle->position.y = origin_y;
                    obstacle->bottom = origin_z - (origin_top - extent);
                    obstacle->height = (origin_top + origin_top) - (extent + extent);
                    if (obstacle->height < 0.0f) {
                        obstacle->height = 0.0f;
                    }
                }
            }
        }
        cursor++;
        remaining--;
    } while (remaining != 0);
}

#if 0
Original Ghidra decompilation (0x418ce0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void actor_movement_collect_obstacle_candidates(int param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  undefined4 uVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  ushort uVar11;
  short sVar12;
  int iVar13;
  short *psVar14;
  int iVar15;
  float *pfVar16;
  float local_2074;
  uint *local_2070;
  uint local_2068;
  float local_204c;
  float local_2048;
  float local_2040 [14];
  uint local_2008 [2047];
  undefined4 uStack_c;

  uStack_c = 0x418cf0;
  uVar11 = object_find_in_sphere
                     (1,0xc2,*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                     (*(uint *)(param_1 + 8) & 0xffff) * 0xc) + 0x98,param_1 + 0xc,
                      *(undefined4 *)(param_1 + 0x6044),local_2008,0x800);
  *(undefined2 *)(param_1 + 0x3c) = 0;
  if (0 < (short)uVar11) {
    local_2068 = (uint)uVar11;
    local_2070 = local_2008;
    do {
      uVar1 = *local_2070;
      if ((uVar1 != 0xffffffff) && (uVar1 != *(uint *)(param_1 + 8))) {
        iVar15 = (uVar1 & 0xffff) * 0xc;
        iVar2 = *(int *)((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar15
                                                         ) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                   0x7c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if (0 < *(int *)(iVar2 + 0x280)) {
          iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar15);
          fVar4 = *(float *)(iVar3 + 0xa0);
          fVar5 = *(float *)(iVar3 + 0xac);
          fVar6 = *(float *)(iVar3 + 0xa4);
          uVar7 = *(undefined4 *)(iVar3 + 0xa8);
          local_2074 = 0.0;
          object_get_world_matrix();
          iVar3 = *(int *)(iVar2 + 0x280);
          sVar12 = 0;
          if (0 < iVar3) {
            iVar2 = *(int *)(iVar2 + 0x284);
            iVar13 = 0;
            do {
              psVar14 = (short *)(iVar13 * 0x20 + iVar2);
              if (*psVar14 == -1) {
                matrix4x3_transform_point(local_2040);
                fVar8 = local_2040[0] * *(float *)(psVar14 + 0xe);
              }
              else {
                iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar15);
                pfVar16 = (float *)((int)*(short *)(iVar13 + 0x1f2) + *psVar14 * 0x34 + iVar13);
                matrix4x3_transform_point(pfVar16);
                fVar8 = *(float *)(psVar14 + 0xe) * *pfVar16;
              }
              fVar9 = local_204c - fVar4;
              fVar10 = local_2048 - fVar6;
              fVar8 = SQRT(fVar10 * fVar10 + fVar9 * fVar9) + fVar8;
              if (local_2074 <= fVar8) {
                local_2074 = fVar8;
              }
              sVar12 = sVar12 + 1;
              iVar13 = (int)sVar12;
            } while (iVar13 < iVar3);
          }
          sVar12 = *(short *)(param_1 + 0x3c);
          if (sVar12 < 0x400) {
            *(short *)(param_1 + 0x3c) = sVar12 + 1;
            iVar2 = param_1 + sVar12 * 0x18;
            *(uint *)(iVar2 + 0x40) = *local_2070;
            *(float *)(iVar2 + 0x54) = local_2074;
            *(float *)(iVar2 + 0x44) = fVar4;
            *(float *)(iVar2 + 0x48) = fVar6;
            *(undefined4 *)(iVar2 + 0x4c) = uVar7;
            *(float *)(iVar2 + 0x4c) = *(float *)(iVar2 + 0x4c) - (fVar5 - local_2074);
            fVar4 = (fVar5 + fVar5) - (local_2074 + local_2074);
            if (fVar4 < 0.0) {
              fVar4 = 0.0;
            }
            *(float *)(iVar2 + 0x50) = fVar4;
          }
        }
      }
      local_2070 = local_2070 + 1;
      local_2068 = local_2068 - 1;
    } while (local_2068 != 0);
  }
  return;
}
#endif
