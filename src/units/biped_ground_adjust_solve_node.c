// biped_ground_adjust_solve_node  (Ghidra: biped_ground_adjust_solve_node, renamed)
// address 0x557b80, size 1143 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: node hierarchy walked through ModelAnimationsAnimationGraphNode (tags.h, stride
//   0x40: parent_node_index 0x24, node_joint_flags 0x28 -- bit 0x2 "hinge", bit 0x4
//   "no_movement" per its bitfield comment); node transforms are real_matrix4x3 (math.h,
//   stride 0x34: up 0x1c, position 0x28), matching object.nodes ("an array of real_matrix4x3",
//   objects.h). Object tag chain and success bitset match the sibling functions in this
//   cluster (0x557a90, 0x558000).
// register convention: object index in EAX, a second position pointer in EBX (loaded once by
//   an outer caller and carried unmodified through the whole 0x557a90/0x558000/0x557b80 call
//   chain -- Ghidra shows it "unaff_EBX" here because nothing in this function reloads it).
//   node_index, nodes, own_position and success_bits are Ghidra-recognized stack parameters.
//   // blam-cc: EAX -> object_index, EBX -> reference_position, stack -> node_index, nodes,
//   //           own_position, success_bits
// UNSURE: reference_position's exact meaning (its caller is outside this batch); every
//   ABS(x - 1.0) < 0.0001 test is a "these two directions already agree" cosine check and is
//   preserved as-is rather than renamed to a helper. plane3d_from_point_and_normal, physics_point_refresh_leaf,
//   matrix4x3_inverse/_transform_vector, vector3d_rotate_about_axis and FUN_00628140 belong to
//   other not-yet-rewritten modules (math/physics); declared here with the signatures their
//   call sites imply.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;    // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place, returns length, vector in ECX (verified: src/objects/object_set_position_and_orientation.c)
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0, out=stack_operand x ecx_operand (verified: src/objects/object_set_position_and_orientation.c)
extern void plane3d_from_point_and_normal(real_plane3d *out, const real_vector3d *normal, const real_point3d *point);
    // 0x44d9e0, src/math; blam-cc: stack out, ECX normal, EDX point
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0, UNSURE signature
// matrix4x3_transform_vector (0x4cbe50) transforms the vector in one register by the matrix in
// another and writes the result through the third; Ghidra binds a different subset at each call
// site in this module, so the declaration is left unprototyped.
extern void matrix4x3_transform_vector();
// vector3d_rotate_about_axis (0x4cd820) rotates the vector in EAX about the axis in ECX in
// place, by the (sin_angle, cos_angle) pair pushed on the stack -- the callee own
// decompilation is a Rodrigues formula over in_EAX / in_ECX / param_1 / param_2, and
// src/math/vector3d_rotate_toward.c reads it the same way. Ghidra binds only the two stack
// arguments at the call sites below, so the declaration is left unprototyped.
extern void vector3d_rotate_about_axis(); // 0x4cd820
  // real signature (vector3d_rotate_about_axis.c): void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); Ghidra recovered 0 of 4 args at this call site
extern char physics_point_refresh_leaf(float threshold); // 0x505540, UNSURE signature/module
extern float FUN_00628140(void);           // 0x628140, UNSURE signature/module (returns an angle)

// Computes a candidate ground-adjusted world position for skeleton node node_index against its
// parent node's basis, validating it with either a plane-rotation test (default) or a
// hinge-relative test (node_joint_flags bit 0x2), and records success as bit node_index in the
// success_bits array. Returns 1 if own_position was updated with a validated position.
// UNSURE: the exact geometric meaning of reference_position; see file header.
char biped_ground_adjust_solve_node(uint32_t object_index, real_point3d *reference_position,
                                     int32_t node_index, real_matrix4x3 *nodes,
                                     real_point3d *own_position, uint32_t *success_bits)
{
    Object *object_tag;
    ModelAnimations *graph;
    ModelAnimationsAnimationGraphNode *graph_nodes;
    int32_t parent_index;
    ModelAnimationsAnimationGraphNode *self_node;
    ModelAnimationsAnimationGraphNode *parent_node;
    float tolerance;
    char updated = 0;
    real_matrix4x3 *self_transform;
    real_matrix4x3 *parent_transform;

    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    graph = (ModelAnimations *)tag_instances[object_tag->animation_graph.tag_id.index].data;
    graph_nodes = (ModelAnimationsAnimationGraphNode *)graph->nodes.pointer;
    tolerance = graph->limp_body_node_radius;
    if ((tolerance < 0.0001f && tolerance > -0.0001f) || tolerance < 0.0f || tolerance > 0.07f) {
        tolerance = 0.03f;
    }

    self_node = &graph_nodes[node_index];
    parent_index = (int16_t)self_node->parent_node_index; // sign-extended, as Ghidra reads it (short)
    parent_node = &graph_nodes[parent_index];
    self_transform = &nodes[node_index];
    parent_transform = &nodes[parent_index];

    if (parent_index != 0) {
        if ((parent_node->node_joint_flags & 4) == 0) {
            real_vector3d self_to_parent, self_to_ref, up_delta, resolved_axis;
            float alignment;

            self_to_parent.i = self_transform->position.x - parent_transform->position.x;
            self_to_parent.j = self_transform->position.y - parent_transform->position.y;
            self_to_parent.k = self_transform->position.z - parent_transform->position.z;
            self_to_ref.i = reference_position->x - parent_transform->position.x;
            self_to_ref.j = reference_position->y - parent_transform->position.y;
            self_to_ref.k = reference_position->z - parent_transform->position.z;

            vector3d_normalize_with_length(&self_to_parent);
            vector3d_normalize_with_length(&self_to_ref);
            vector3d_cross_product(&up_delta, &self_to_parent, 0);
            vector3d_normalize_with_length(&up_delta);
            alignment = self_to_parent.i * self_to_ref.i + self_to_parent.j * self_to_ref.j +
                        self_to_parent.k * self_to_ref.k;

            if (alignment - 1.0f >= 0.0001f || alignment - 1.0f <= -0.0001f) {
                double angle = FUN_00628140();
                real_matrix4x3 inverse_a, inverse_b;
                real_vector3d up_from_a, up_from_b;

                matrix4x3_inverse(&inverse_a, parent_transform);
                matrix4x3_inverse(&inverse_b, parent_transform);
                matrix4x3_transform_vector(&up_from_a, &inverse_a, &parent_transform->up);

                if ((parent_node->node_joint_flags & 2) == 0) {
                    matrix4x3_transform_vector(&up_from_b, &inverse_b, &parent_transform->up);
                    vector3d_rotate_about_axis(&resolved_axis, (float)angle, &up_delta, &self_to_parent);
                    alignment = up_from_a.i * parent_node->base_vector.i +
                                up_from_a.k * parent_node->base_vector.k +
                                up_from_a.j * parent_node->base_vector.j;
                    if ((alignment - 1.0f >= 0.0001f || alignment - 1.0f <= -0.0001f) &&
                        (angle = FUN_00628140(), (angle < 0 ? -angle : angle) < parent_node->vector_range) &&
                        reference_position->z < own_position->z) {
                        success_bits[node_index >> 5] |= 1u << (node_index & 0x1f);
                        *own_position = *reference_position;
                        updated = 1;
                    }
                } else {
                    real_vector3d hinge_delta;
                    float hinge_scale;
                    real_vector3d hinge_point;
                    real_plane3d hinge_plane;

                    hinge_delta.i = *(float *)((uint8_t *)parent_transform + 0x1c);
                    hinge_delta.j = *(float *)((uint8_t *)parent_transform + 0x20);
                    hinge_delta.k = *(float *)((uint8_t *)parent_transform + 0x24);
                    // 0x557d82..0x557daa: out = local plane, ECX = &hinge_delta (the parent's up,
                    // parent_transform + 0x1c), EDX = &parent_transform->position ([esp+0x30])
                    plane3d_from_point_and_normal(&hinge_plane, &hinge_delta, &parent_transform->position);
                    // 0x557daf..0x557e0b: reference_position (EBX) projected onto that plane along
                    // hinge_delta. The earlier rewrite used self_to_parent with its components
                    // crossed; the asm reads [ebx], [ebx+4], [ebx+8] in step with the normal.
                    hinge_scale = -((hinge_plane.normal.j * reference_position->y +
                                     hinge_plane.normal.i * reference_position->x +
                                     hinge_plane.normal.k * reference_position->z) - hinge_plane.d);
                    hinge_point.i = hinge_delta.i * hinge_scale + reference_position->x;
                    hinge_point.j = hinge_delta.j * hinge_scale + reference_position->y;
                    hinge_point.k = hinge_delta.k * hinge_scale + reference_position->z;
                    self_to_ref.i = hinge_point.i - parent_transform->position.x;
                    self_to_ref.j = hinge_point.j - parent_transform->position.y;
                    self_to_ref.k = hinge_point.k - parent_transform->position.z;
                    vector3d_normalize_with_length(&self_to_ref);
                    matrix4x3_transform_vector(&up_from_a, &inverse_a, &parent_transform->up);
                    alignment = up_from_a.i * parent_node->base_vector.i +
                                up_from_a.k * parent_node->base_vector.k +
                                up_from_a.j * parent_node->base_vector.j;
                    if (alignment - 1.0f >= 0.0001f || alignment - 1.0f <= -0.0001f) {
                        success_bits[node_index >> 5] |= 1u << (node_index & 0x1f);
                        if (hinge_point.k <= own_position->z && !physics_point_refresh_leaf(tolerance)) {
                            own_position->x = hinge_point.i;
                            own_position->y = hinge_point.j;
                            own_position->z = hinge_point.k;
                        }
                        updated = 1;
                    }
                }
            }
        }
    }

    if ((parent_node->node_joint_flags & 4) == 0 && updated) {
        return updated;
    }

    if (((success_bits[parent_index >> 5] &
          (1u << (parent_index & 0x1f))) != 0) &&
        (reference_position->z < own_position->z)) {
        success_bits[node_index >> 5] |= 1u << (node_index & 0x1f);
        *own_position = *reference_position;
        return 1;
    }
    return updated;
}

#if 0
Original Ghidra decompilation (0x557b80):

char FUN_00557b80(int param_1,int param_2,float *param_3,int param_4)

{
  uint *puVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  short sVar9;
  byte bVar10;
  char cVar11;
  uint in_EAX;
  int iVar12;
  int iVar13;
  float *unaff_EBX;
  int iVar14;
  int iVar15;
  float10 fVar16;
  char local_b5;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  float local_98;
  int local_94;
  float *local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  undefined1 local_78 [56];
  undefined1 local_40 [60];

  iVar12 = *(int *)((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                                    (in_EAX & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14
                                       + DAT_0087bc14) + 0x44) & 0xffff) * 0x20 + 0x14 +
                   DAT_0087bc14);
  iVar14 = *(int *)(iVar12 + 0x6c);
  local_98 = *(float *)(iVar12 + 0x60);
  iVar12 = (int)*(short *)(param_1 * 0x40 + 0x24 + iVar14);
  iVar15 = param_1 * 0x40 + iVar14;
  iVar14 = iVar12 * 0x40 + iVar14;
  local_b5 = '\0';
  if (((ABS(local_98) < 0.0001) || (local_98 < 0.0)) || (0.07 < local_98)) {
    local_98 = 0.03;
  }
  iVar13 = param_1 >> 5;
  bVar10 = (byte)param_1;
  if (*(short *)(iVar15 + 0x24) != 0) {
    if ((*(byte *)(iVar14 + 0x28) & 4) != 0) goto LAB_00557f99;
    iVar2 = param_1 * 0x34 + 0x28 + param_2;
    local_94 = param_2 + iVar12 * 0x34;
    local_90 = (float *)(local_94 + 0x28);
    local_a8 = *(float *)(param_1 * 0x34 + 0x28 + param_2) - *local_90;
    local_a4 = *(float *)(iVar2 + 4) - *(float *)(local_94 + 0x2c);
    local_a0 = *(float *)(iVar2 + 8) - *(float *)(local_94 + 0x30);
    fVar3 = *unaff_EBX;
    fVar4 = *local_90;
    fVar5 = unaff_EBX[1];
    fVar6 = *(float *)(local_94 + 0x2c);
    fVar7 = unaff_EBX[2];
    fVar8 = *(float *)(local_94 + 0x30);
    vector3d_normalize_with_length();
    vector3d_normalize_with_length();
    vector3d_cross_product(&local_a8);
    vector3d_normalize_with_length();
    local_9c = (fVar3 - fVar4) * local_a8 + (fVar5 - fVar6) * local_a4 + (fVar7 - fVar8) * local_a0;
    if (0.0001 <= ABS(local_9c - 1.0)) {
      fVar16 = (float10)FUN_00628140();
      local_7c = (float)fVar16;
      matrix4x3_inverse();
      matrix4x3_inverse();
      matrix4x3_transform_vector(local_78);
      fVar3 = *(float *)(local_94 + 4);
      fVar4 = *(float *)(local_94 + 8);
      fVar5 = *(float *)(local_94 + 0xc);
      matrix4x3_transform_vector(local_40);
      if ((*(byte *)(iVar14 + 0x28) & 2) == 0) {
        fVar16 = (float10)fsin((float10)local_7c);
        vector3d_rotate_about_axis((float)fVar16,local_9c);
        local_9c = fVar3 * *(float *)(iVar14 + 0x2c) +
                   fVar5 * *(float *)(iVar14 + 0x34) + fVar4 * *(float *)(iVar14 + 0x30);
        if (((0.0001 <= ABS(local_9c - 1.0)) &&
            (fVar16 = (float10)FUN_00628140(), ABS(fVar16) < (float10)*(float *)(iVar14 + 0x38))) &&
           (unaff_EBX[2] < param_3[2])) {
          puVar1 = (uint *)(param_4 + iVar13 * 4);
          *puVar1 = *puVar1 | 1 << (bVar10 & 0x1f);
          *param_3 = *unaff_EBX;
          param_3[1] = unaff_EBX[1];
          param_3[2] = unaff_EBX[2];
          goto LAB_00557f7a;
        }
      }
      else {
        local_a8 = *(float *)(local_94 + 0x1c);
        local_a4 = *(float *)(local_94 + 0x20);
        local_a0 = *(float *)(local_94 + 0x24);
        FUN_0044d9e0(&local_8c);
        fVar3 = ((local_84 * unaff_EBX[2] + local_8c * *unaff_EBX + local_88 * unaff_EBX[1]) -
                local_80) * -1.0;
        fVar5 = local_a8 * fVar3 + *unaff_EBX;
        fVar4 = local_a4 * fVar3 + unaff_EBX[1];
        fVar3 = local_a0 * fVar3 + unaff_EBX[2];
        local_a8 = fVar5 - *local_90;
        local_a4 = fVar4 - local_90[1];
        local_a0 = fVar3 - local_90[2];
        vector3d_normalize_with_length();
        matrix4x3_transform_vector(local_78);
        if (0.0001 <= ABS((local_8c * *(float *)(iVar14 + 0x2c) +
                          local_84 * *(float *)(iVar14 + 0x34) +
                          local_88 * *(float *)(iVar14 + 0x30)) - 1.0)) {
          puVar1 = (uint *)(param_4 + iVar13 * 4);
          *puVar1 = *puVar1 | 1 << (bVar10 & 0x1f);
          if ((fVar3 <= param_3[2]) && (cVar11 = FUN_00505540(local_98), cVar11 == '\0')) {
            *param_3 = fVar5;
            param_3[1] = fVar4;
            param_3[2] = fVar3;
          }
LAB_00557f7a:
          local_b5 = '\x01';
        }
      }
    }
  }
  if (((*(byte *)(iVar14 + 0x28) & 4) == 0) && (local_b5 != '\0')) {
    return local_b5;
  }
LAB_00557f99:
  sVar9 = *(short *)(iVar15 + 0x24);
  if (((*(uint *)(param_4 + ((int)sVar9 >> 5) * 4) & 1 << ((byte)sVar9 & 0x1f)) != 0) &&
     (unaff_EBX[2] < param_3[2])) {
    puVar1 = (uint *)(param_4 + iVar13 * 4);
    *puVar1 = *puVar1 | 1 << (bVar10 & 0x1f);
    *param_3 = *unaff_EBX;
    param_3[1] = unaff_EBX[1];
    param_3[2] = unaff_EBX[2];
    return '\x01';
  }
  return local_b5;
}
#endif
