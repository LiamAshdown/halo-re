// biped_ground_adjust_solve  (Ghidra: biped_ground_adjust_solve, renamed)
// address 0x558000, size 2123 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: same object/tag/graph-node chain as 0x557a90 and 0x557b80 (types/units.h,
//   ModelAnimationsAnimationGraphNode); the per-node "rest length" it compares against is
//   GBXModel.nodes[i].node_distance_from_parent (tags.h ModelNode, reached through
//   Object.model, confirmed by the 0x9c stride and the 0x44 field offset matching that
//   struct exactly) -- this is a bone-length (distance) constraint layered on top of the
//   ground-contact solve that 0x557b80 performs per node.
// register convention: object index recognized as a normal (stack) parameter; EBX is
//   "unaff" here too (see 0x557a90's file header) and is threaded through explicitly as
//   reference_position, purely to reach 0x557b80 three calls down.
//   // blam-cc: stack -> object_index, nodes; EBX (from the original caller) -> reference_position
// UNSURE: physics_model_build_from_sphere_query, physics_model_slide_along_contacts, physics_point_refresh_leaf and collision_test_movement_segment are unresolved
//   collision/physics-module calls; their arguments and the scratch buffers they fill
//   (probe_origin, hit_result, plane_result, collision_scratch) are preserved with the exact
//   byte layout Ghidra assigned them, not renamed to an invented meaning. The two "which side"
//   bytes and the plane-distance arithmetic strongly suggest collision_test_movement_segment returns a separating
//   plane and physics_point_refresh_leaf is a cheap per-node skip/throttle test, but this is not certain.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern char physics_point_refresh_leaf(float threshold); // 0x505540, UNSURE signature/module
extern char collision_test_movement_segment(uint32_t flags, real_point3d *segment_start, real_vector3d *segment_delta,
                          uint32_t object_index, void *plane_result); // 0x505880, UNSURE signature/module
extern void physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center, float radius, uint32_t unused_zero,
                          float tolerance, uint32_t object_index, real_point3d *out_origin); // 0x506440, UNSURE signature/module
extern void physics_model_slide_along_contacts(real_vector3d *direction, real_point3d *origin, void *hit_result_position,
                          void *hit_result_extra, uint32_t axis_count, void *scratch); // 0x5067b0, UNSURE signature/module

extern double sqrt(double x); // a single x87 FSQRT instruction in the original (Ghidra's SQRT())
extern char biped_ground_adjust_solve_node(uint32_t object_index, real_point3d *reference_position,
                                            int32_t node_index, real_matrix4x3 *nodes,
                                            real_point3d *own_position, uint32_t *success_bits); // 0x557b80

// Walks the node tree of a biped's skeleton up to four times (breadth-first from the root),
// applying a bone-length (distance) constraint between each node and its parent, and -- on the
// first pass only, throttled by physics_point_refresh_leaf -- a ground-contact solve via
// biped_ground_adjust_solve_node. Runs only while the biped's ground-adjust iteration count is
// still below its tag-seeded limit.
void biped_ground_adjust_solve(uint32_t object_index, real_point3d *reference_position, real_matrix4x3 *nodes)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    Object *object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    ModelAnimations *graph = (ModelAnimations *)tag_instances[object_tag->animation_graph.tag_id.index].data;
    ModelAnimationsAnimationGraphNode *graph_nodes = (ModelAnimationsAnimationGraphNode *)graph->nodes.pointer;
    GBXModel *model = (GBXModel *)tag_instances[object_tag->model.tag_id.index].data;
    ModelNode *model_nodes = (ModelNode *)model->nodes.pointer;
    float tolerance = graph->limp_body_node_radius;
    uint8_t iteration_limit;
    float progress;

    if ((tolerance < 0.0001f && tolerance > -0.0001f) || tolerance < 0.0f || tolerance > 0.07f) {
        tolerance = 0.03f;
    }

    iteration_limit = biped->ground_adjust_iteration_limit;
    if (iteration_limit == 0 || iteration_limit >= 0x1e) {
        return;
    }
    progress = (float)(biped->ground_adjust_iteration + 1) / (float)iteration_limit;
    if (!(progress >= 0.0001f || progress <= -0.0001f) || biped->ground_adjust_iteration >= iteration_limit) {
        return;
    }

    {
        real_point3d probe_origin;      // UNSURE: DAT_006e4d08, filled once below
        uint32_t success_bits[2] = {0, 0}; // local_264/local_260; a node-index bitset (64 nodes worst case)
        int32_t pass;

        physics_model_build_from_sphere_query(0xc0a8, &obj->position, obj->bounding_radius + 0.0625f, 0, tolerance, object_index,
                     &probe_origin);

        for (pass = 0; pass < 4; pass++) {
            int16_t stack[64];
            int16_t read_index = 0;
            int16_t write_index = 1;
            stack[0] = 0;

            do {
                int16_t node_index = stack[read_index];
                ModelAnimationsAnimationGraphNode *self_tag_node = &graph_nodes[node_index];
                read_index = read_index + 1;

                if (node_index != 0) {
                    int32_t parent_index = (int16_t)self_tag_node->parent_node_index;
                    real_point3d *self_position = &nodes[node_index].position;
                    real_point3d *parent_position = &nodes[parent_index].position;
                    real_vector3d probe_delta;
                    real_vector3d delta;
                    float hit_result_position[3];
                    float hit_result_extra[2];
                    uint8_t collision_scratch[440];

                    probe_delta.i = 0.0f;
                    probe_delta.j = 0.0f;
                    probe_delta.k = self_position->z - parent_position->z;

                    if (pass == 0 && !physics_point_refresh_leaf(tolerance)) {
                        physics_model_slide_along_contacts(&probe_delta, &probe_origin, hit_result_position, hit_result_extra, 3,
                                     collision_scratch);
                        if ((hit_result_extra[0] < 0.0001f && hit_result_extra[0] > -0.0001f) &&
                            (hit_result_extra[1] < 0.0001f && hit_result_extra[1] > -0.0001f)) {
                            biped_ground_adjust_solve_node(object_index, reference_position, node_index, nodes,
                                                            self_position, success_bits);
                        }
                    }

                    {
                        real_point3d segment_start;
                        real_vector3d segment_delta;
                        // UNSURE: Ghidra's stack layout puts this immediately after a 36-byte buffer
                        // (local_2b4) that collision_test_movement_segment fills as its 5th (output) argument; the
                        // decompiler's naming suggests the plane normal/d it reads afterward alias the
                        // front of that same buffer, but the true field layout is unresolved -- kept as
                        // its own scratch array rather than guessing at real_plane3d.
                        float plane_result[9];
                        uint8_t which_side[4];

                        segment_start.x = parent_position->x - (self_position->x - parent_position->x) * 0.015f;
                        segment_start.y = parent_position->y - (self_position->y - parent_position->y) * 0.015f;
                        segment_start.z = parent_position->z - (self_position->z - parent_position->z) * 0.015f;
                        segment_delta.i = (self_position->x - parent_position->x) * 1.03f;
                        segment_delta.j = (self_position->y - parent_position->y) * 1.03f;
                        segment_delta.k = (self_position->z - parent_position->z) * 1.03f;

                        if (collision_test_movement_segment(0xc0a8, &segment_start, &segment_delta, object_index, plane_result)) {
                            real_vector3d plane_normal;
                            float plane_d;
                            float side_offset[2];
                            int32_t side;

                            plane_normal.i = plane_result[0]; // UNSURE: fStack_290
                            plane_normal.j = plane_result[1]; // UNSURE: fStack_28c
                            plane_normal.k = plane_result[2]; // UNSURE: fStack_288
                            plane_d       = plane_result[3];  // UNSURE: fStack_284
                            which_side[0] = (uint8_t)physics_point_refresh_leaf(0.03f);
                            which_side[1] = (uint8_t)physics_point_refresh_leaf(0.03f);

                            if (which_side[0] != 0 || which_side[1] != 0) {
                                if (which_side[0] != 0 && which_side[1] != 0) {
                                    float len_sq = plane_normal.i * plane_normal.i + plane_normal.j * plane_normal.j +
                                                   plane_normal.k * plane_normal.k;
                                    side_offset[0] = -(((plane_normal.j * self_position->y +
                                                          plane_normal.k * self_position->z +
                                                          plane_normal.i * self_position->x) - plane_d) / len_sq);
                                    if (side_offset[0] != 0.0f) side_offset[0] = tolerance * 2.5f + side_offset[0];
                                    side_offset[1] = -(((plane_normal.k * parent_position->z +
                                                          plane_normal.j * parent_position->y +
                                                          plane_normal.i * parent_position->x) - plane_d) / len_sq);
                                    if (side_offset[1] != 0.0f) side_offset[1] = tolerance * 2.5f + side_offset[1];
                                } else {
                                    for (side = 0; side < 2; side++) {
                                        if (which_side[side] == 0) {
                                            side_offset[side] = 0.0f;
                                        } else {
                                            real_point3d *p = (side == 0) ? self_position : parent_position;
                                            float len_sq = plane_normal.i * plane_normal.i +
                                                           plane_normal.j * plane_normal.j +
                                                           plane_normal.k * plane_normal.k;
                                            side_offset[side] = -(((plane_normal.i * p->x + plane_normal.k * p->z +
                                                                     plane_normal.j * p->y) - plane_d) / len_sq);
                                            if (side_offset[side] != 0.0f) {
                                                side_offset[side] = tolerance * 2.5f + side_offset[side];
                                            }
                                        }
                                    }
                                }
                                if (side_offset[0] >= 0.0001f || side_offset[0] <= -0.0001f) {
                                    float s = side_offset[0] * progress;
                                    self_position->x = plane_normal.i * s + self_position->x;
                                    self_position->y = plane_normal.j * s + self_position->y;
                                    self_position->z = plane_normal.k * s + self_position->z;
                                }
                                if (side_offset[1] >= 0.0001f || side_offset[1] <= -0.0001f) {
                                    float s = side_offset[1] * progress;
                                    parent_position->x = plane_normal.i * s + parent_position->x;
                                    parent_position->y = plane_normal.j * s + parent_position->y;
                                    parent_position->z = plane_normal.k * s + parent_position->z;
                                }
                            }
                        }
                    }

                    {
                        float rest_length = model_nodes[node_index].node_distance_from_parent;
                        float current_length = (float)sqrt(
                            (parent_position->x - self_position->x) * (parent_position->x - self_position->x) +
                            (parent_position->y - self_position->y) * (parent_position->y - self_position->y) +
                            (parent_position->z - self_position->z) * (parent_position->z - self_position->z));

                        if (rest_length >= 0.0f && rest_length <= 10.0f && current_length >= 0.0f &&
                            current_length < 20.0f && (current_length >= 0.0001f || current_length <= -0.0001f)) {
                            if ((rest_length >= 0.0001f || rest_length <= -0.0001f) &&
                                rest_length != current_length &&
                                (current_length >= 0.0001f || current_length <= -0.0001f)) {
                                float stretch = (rest_length - current_length) / current_length;
                                real_vector3d bone_delta;
                                bone_delta.i = self_position->x - parent_position->x;
                                bone_delta.j = self_position->y - parent_position->y;
                                bone_delta.k = self_position->z - parent_position->z;

                                if (parent_index == 0) { // UNSURE: original re-reads the same parent_node_index
                                    real_vector3d correction;
                                    correction.i = bone_delta.i * stretch;
                                    correction.j = bone_delta.j * stretch;
                                    correction.k = bone_delta.k * stretch;
                                    if (!physics_point_refresh_leaf(tolerance)) {
                                        physics_model_slide_along_contacts(&correction, &probe_origin, self_position, &correction, 3,
                                                     collision_scratch);
                                    }
                                } else {
                                    real_vector3d correction;
                                    float half = stretch * 0.5f;
                                    correction.i = bone_delta.i * -half;
                                    correction.j = bone_delta.j * -half;
                                    correction.k = bone_delta.k * -half;
                                    physics_model_slide_along_contacts(&correction, &probe_origin, parent_position, &correction, 3,
                                                 collision_scratch);
                                    correction.i = bone_delta.i * half;
                                    correction.j = bone_delta.j * half;
                                    correction.k = bone_delta.k * half;
                                    physics_model_slide_along_contacts(&correction, &probe_origin, self_position, &correction, 3,
                                                 collision_scratch);
                                }
                            }
                            /* both branches fall through to pushing this node's children below,
                               matching the original's "goto LAB_005587f7" (see file header) */
                        }
                    }
                }

                if (self_tag_node->first_child_node_index != 0xffff) {
                    stack[write_index] = (int16_t)self_tag_node->first_child_node_index;
                    write_index = write_index + 1;
                }
                if (self_tag_node->next_sibling_node_index != 0xffff) {
                    stack[write_index] = (int16_t)self_tag_node->next_sibling_node_index;
                    write_index = write_index + 1;
                }
            } while (read_index != write_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x558000):

void FUN_00558000(uint param_1,int param_2)

{
  float *pfVar1;
  float *pfVar2;
  float fVar3;
  uint *puVar4;
  char cVar5;
  byte bVar6;
  int iVar7;
  float *pfVar8;
  short sVar9;
  int iVar10;
  short sVar11;
  int iVar12;
  float local_2fc;
  float local_2f4;
  float local_2f0;
  float local_2ec;
  uint local_2e8;
  float afStack_2e4 [2];
  int local_2dc;
  float fStack_2d8;
  float local_2d4;
  float local_2d0;
  float local_2cc;
  byte abStack_2c8 [4];
  float local_2c4;
  int local_2c0;
  int local_2bc;
  int local_2b8;
  undefined1 local_2b4 [36];
  float fStack_290;
  float fStack_28c;
  float fStack_288;
  float fStack_284;
  undefined4 local_264;
  undefined4 local_260;
  float local_25c;
  float fStack_258;
  float fStack_254;
  float local_250;
  float fStack_24c;
  undefined1 local_244 [12];
  short local_238 [64];
  undefined1 local_1b8 [440];

  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  local_2bc = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_2b8 = *(int *)((*(uint *)(local_2bc + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_2fc = *(float *)(local_2b8 + 0x60);
  if (((ABS(local_2fc) < 0.0001) || (local_2fc < 0.0)) || (0.07 < local_2fc)) {
    local_2fc = 0.03;
  }
  bVar6 = *(byte *)((int)puVar4 + 0x525);
  if ((bVar6 != 0) && (bVar6 < 0x1e)) {
    local_2e8 = (uint)bVar6;
    local_2c4 = (float)((byte)puVar4[0x149] + 1) / (float)local_2e8;
    if ((0.0001 <= ABS(local_2c4)) && ((byte)puVar4[0x149] < bVar6)) {
      FUN_00506440(0xc0a8,puVar4 + 0x17,(float)puVar4[0x2b] + 0.0625,0,local_2fc,param_1,
                   &DAT_006e4d08);
      local_264 = 0;
      local_260 = 0;
      local_2e8 = 0;
      do {
        local_2c0 = 0;
        sVar11 = 1;
        local_238[0] = 0;
        do {
          sVar9 = (short)local_2c0;
          local_2c0 = local_2c0 + 1;
          iVar12 = (int)local_238[sVar9];
          local_2dc = iVar12 * 0x40 + *(int *)(local_2b8 + 0x6c);
          if (local_238[sVar9] == 0) {
LAB_005587f7:
            if (*(short *)(local_2dc + 0x20) != -1) {
              iVar12 = (int)sVar11;
              sVar11 = sVar11 + 1;
              local_238[iVar12] = *(short *)(local_2dc + 0x20);
            }
            if (*(short *)(local_2dc + 0x22) != -1) {
              iVar12 = (int)sVar11;
              sVar11 = sVar11 + 1;
              local_238[iVar12] = *(short *)(local_2dc + 0x22);
            }
          }
          else {
            local_2cc = local_2c4 * -0.032086615;
            iVar10 = *(short *)(local_2dc + 0x24) * 0x34;
            iVar7 = iVar12 * 0x34;
            local_2f4 = *(float *)(iVar7 + 0x28 + param_2) - *(float *)(iVar10 + 0x28 + param_2);
            local_2f0 = *(float *)(iVar7 + 0x2c + param_2) - *(float *)(iVar10 + 0x2c + param_2);
            pfVar1 = (float *)(iVar7 + 0x28 + param_2);
            pfVar2 = (float *)(iVar10 + 0x28 + param_2);
            local_2d4 = 0.0;
            local_2d0 = 0.0;
            local_2ec = pfVar1[2] - pfVar2[2];
            if (((local_2e8 == 0) && (cVar5 = FUN_00505540(local_2fc), cVar5 == '\0')) &&
               ((FUN_005067b0(&local_2d4,&DAT_006e4d08,local_244,&local_250,3,local_1b8),
                ABS(local_250) < 0.0001 && (ABS(fStack_24c) < 0.0001)))) {
              FUN_00557b80(iVar12,param_2,pfVar1,&local_264);
            }
            local_25c = *pfVar2 - (*pfVar1 - *pfVar2) * 0.015;
            fStack_258 = pfVar2[1] - (pfVar1[1] - pfVar2[1]) * 0.015;
            fStack_254 = pfVar2[2] - (pfVar1[2] - pfVar2[2]) * 0.015;
            local_2f4 = (*pfVar1 - *pfVar2) * 1.03;
            local_2f0 = (pfVar1[1] - pfVar2[1]) * 1.03;
            local_2ec = (pfVar1[2] - pfVar2[2]) * 1.03;
            cVar5 = FUN_00505880(0xc0a8,&local_25c,&local_2f4,param_1,local_2b4);
            if (cVar5 != '\0') {
              bVar6 = FUN_00505540(0x3cf5c28f);
              abStack_2c8[0] = bVar6;
              abStack_2c8[1] = FUN_00505540(0x3cf5c28f);
              if ((uint)abStack_2c8[1] + (uint)bVar6 != 0) {
                if ((uint)abStack_2c8[1] + (uint)bVar6 == 2) {
                  fVar3 = fStack_290 * fStack_290 +
                          fStack_288 * fStack_288 + fStack_28c * fStack_28c;
                  afStack_2e4[0] =
                       -(((fStack_28c * pfVar1[1] + fStack_288 * pfVar1[2] + fStack_290 * *pfVar1) -
                         fStack_284) / fVar3);
                  if (afStack_2e4[0] != 0.0) {
                    afStack_2e4[0] = local_2fc * 2.5 + afStack_2e4[0];
                  }
                  afStack_2e4[1] =
                       -(((fStack_288 * pfVar2[2] + fStack_28c * pfVar2[1] + fStack_290 * *pfVar2) -
                         fStack_284) / fVar3);
                  if (afStack_2e4[1] != 0.0) {
                    afStack_2e4[1] = local_2fc * 2.5 + afStack_2e4[1];
                  }
                }
                else {
                  iVar7 = 0;
                  do {
                    if (abStack_2c8[iVar7] == 0) {
                      afStack_2e4[iVar7] = 0.0;
                    }
                    else {
                      pfVar8 = pfVar1;
                      if (iVar7 != 0) {
                        pfVar8 = pfVar2;
                      }
                      fVar3 = -(((fStack_290 * *pfVar8 +
                                 fStack_288 * pfVar8[2] + fStack_28c * pfVar8[1]) - fStack_284) /
                               (fStack_290 * fStack_290 +
                               fStack_288 * fStack_288 + fStack_28c * fStack_28c));
                      afStack_2e4[iVar7] = fVar3;
                      if (fVar3 != 0.0) {
                        afStack_2e4[iVar7] = local_2fc * 2.5 + fVar3;
                      }
                    }
                    iVar7 = iVar7 + 1;
                  } while (iVar7 < 2);
                }
                if (0.0001 <= ABS(afStack_2e4[0])) {
                  fVar3 = afStack_2e4[0] * local_2c4;
                  *pfVar1 = fStack_290 * fVar3 + *pfVar1;
                  pfVar1[1] = fStack_28c * fVar3 + pfVar1[1];
                  pfVar1[2] = fStack_288 * fVar3 + pfVar1[2];
                }
                if (0.0001 <= ABS(afStack_2e4[1])) {
                  fVar3 = afStack_2e4[1] * local_2c4;
                  *pfVar2 = fStack_290 * fVar3 + *pfVar2;
                  pfVar2[1] = fStack_28c * fVar3 + pfVar2[1];
                  pfVar2[2] = fStack_288 * fVar3 + pfVar2[2];
                }
              }
            }
            local_2f4 = *pfVar1 - *pfVar2;
            local_2f0 = pfVar1[1] - pfVar2[1];
            local_2ec = pfVar1[2] - pfVar2[2];
            fVar3 = *(float *)(*(int *)(*(int *)((*(uint *)(local_2bc + 0x34) & 0xffff) * 0x20 +
                                                 0x14 + DAT_0087bc14) + 0xbc) + 0x44 + iVar12 * 0x9c
                              );
            fStack_2d8 = SQRT((*pfVar2 - *pfVar1) * (*pfVar2 - *pfVar1) +
                              (pfVar2[1] - pfVar1[1]) * (pfVar2[1] - pfVar1[1]) +
                              (pfVar2[2] - pfVar1[2]) * (pfVar2[2] - pfVar1[2]));
            if ((((fVar3 < 0.0 == (fVar3 == 0.0)) && (fVar3 <= 10.0)) && (0.0 <= fStack_2d8)) &&
               ((fStack_2d8 < 20.0 && (0.0001 <= ABS(fStack_2d8))))) {
              if ((0.0001 <= ABS(fVar3)) && ((fVar3 != fStack_2d8 && (0.0001 <= ABS(fStack_2d8)))))
              {
                fVar3 = (fVar3 - fStack_2d8) / fStack_2d8;
                if (*(short *)(local_2dc + 0x24) == 0) {
                  local_2d4 = local_2f4 * fVar3;
                  local_2d0 = local_2f0 * fVar3;
                  local_2cc = local_2ec * fVar3;
                  cVar5 = FUN_00505540(local_2fc);
                  if (cVar5 == '\0') {
                    FUN_005067b0(&local_2d4,&DAT_006e4d08,pfVar1,&local_2d4,3,local_1b8);
                  }
                }
                else {
                  fVar3 = fVar3 * 0.5;
                  local_2cc = -fVar3;
                  local_2d4 = local_2f4 * local_2cc;
                  local_2d0 = local_2f0 * local_2cc;
                  local_2cc = local_2ec * local_2cc;
                  FUN_005067b0(&local_2d4,&DAT_006e4d08,pfVar2,&local_2d4,3,local_1b8);
                  local_2d4 = local_2f4 * fVar3;
                  local_2d0 = local_2f0 * fVar3;
                  local_2cc = local_2ec * fVar3;
                  FUN_005067b0(&local_2d4,&DAT_006e4d08,pfVar1,&local_2d4,3,local_1b8);
                }
              }
              goto LAB_005587f7;
            }
            if (*(short *)(local_2dc + 0x20) != -1) {
              iVar12 = (int)sVar11;
              sVar11 = sVar11 + 1;
              local_238[iVar12] = *(short *)(local_2dc + 0x20);
            }
            if (*(short *)(local_2dc + 0x22) != -1) {
              iVar12 = (int)sVar11;
              sVar11 = sVar11 + 1;
              local_238[iVar12] = *(short *)(local_2dc + 0x22);
            }
          }
        } while ((short)local_2c0 != sVar11);
        local_2e8 = local_2e8 + 1;
      } while ((int)local_2e8 < 4);
    }
  }
  return;
}
#endif
