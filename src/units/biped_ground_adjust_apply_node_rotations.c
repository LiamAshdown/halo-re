// biped_ground_adjust_apply_node_rotations  (Ghidra: biped_ground_adjust_apply_node_rotations, renamed)
// address 0x558a20, size 787 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: same object/tag/graph-node chain as the rest of this cluster (0x557a90, 0x558000,
//   0x557b80); nodes stride 0x34 = real_matrix4x3 (math.h, up at 0x1c, position at 0x28);
//   saved_positions stride 0xc = real_point3d, matching unit_ground_adjust_node_positions
//   (types/units.h) which is what 0x557a90 actually passes here.
// register convention: object index in EAX (Ghidra's in_EAX); nodes and saved_positions are
//   Ghidra-recognized stack parameters.
//   // blam-cc: EAX -> object_index, stack -> nodes, saved_positions
// UNSURE: every vector3d_normalize_with_length / vector3d_cross_product / vector3d_rotate_about_axis
//   call in the original is emitted with NO visible arguments -- Ghidra set up float locals in
//   what must be specific registers/FPU stack slots immediately before each call but could not
//   resolve the calling convention enough to show them as arguments. This rewrite preserves the
//   exact sequence of float assignments and calls and assigns each call the operand its
//   immediately-preceding assignment most plausibly feeds, but the true per-call argument
//   binding cannot be recovered from this decompilation alone. real_matrix4x3_rotation_is_orthonormal and real_matrix4x3_rotation_rebuild_orthonormal
//   are the two math-module helpers this units batch explicitly excludes (orthonormality check
//   and orthonormal-basis rebuild, out/phase4/units_types_notes.md); FUN_00628140 is an
//   unresolved math-module call that returns a small angle.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place, returns length, vector in ECX (verified: src/objects/object_set_position_and_orientation.c)
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0, out=stack_operand x ecx_operand (verified: src/objects/object_set_position_and_orientation.c)
// vector3d_rotate_about_axis (0x4cd820) rotates the vector in EAX about the axis in ECX in
// place, by the (sin_angle, cos_angle) pair pushed on the stack -- the callee own
// decompilation is a Rodrigues formula over in_EAX / in_ECX / param_1 / param_2, and
// src/math/vector3d_rotate_toward.c reads it the same way. Ghidra binds only the two stack
// arguments at the call sites below, so the declaration is left unprototyped.
extern void vector3d_rotate_about_axis(); // 0x4cd820
  // real signature (vector3d_rotate_about_axis.c): void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); Ghidra recovered 0 of 4 args at this call site
extern uint8_t real_matrix4x3_rotation_is_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up);
    // 0x5579e0, src/math; blam-cc: ESI forward, EDI left, EBX up
extern void real_matrix4x3_rotation_rebuild_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up);
    // 0x558860, src/math; blam-cc: ESI forward, EBX left, EDI up
extern float FUN_00628140(void); // 0x628140, UNSURE signature/module (returns an angle)
extern double fsin(double x);     // x87 FSIN

// For every non-root skeleton node whose parent is not flagged "no_movement", compares the
// pre-solve direction from that node to its parent (saved_positions) against the post-solve
// direction (nodes' current positions) and, if they diverge by more than a small angle, rotates
// the parent's local basis to bring them back into agreement -- keeping the parent's basis
// orthonormal via real_matrix4x3_rotation_is_orthonormal / real_matrix4x3_rotation_rebuild_orthonormal when the rotation pushes it out of true.
void biped_ground_adjust_apply_node_rotations(uint32_t object_index, real_matrix4x3 *nodes,
                                               real_point3d *saved_positions)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    ModelAnimations *graph = (ModelAnimations *)tag_instances[object_tag->animation_graph.tag_id.index].data;
    ModelAnimationsAnimationGraphNode *graph_nodes = (ModelAnimationsAnimationGraphNode *)graph->nodes.pointer;
    int32_t i;

    for (i = 0; i < (int32_t)graph->nodes.count; i++) {
        if (i != 0) {
            int32_t parent_index = (int16_t)graph_nodes[i].parent_node_index;
            if ((graph_nodes[parent_index].node_joint_flags & 4) == 0) {
                real_vector3d saved_direction, current_direction, rotation_axis;
                float alignment;

                saved_direction.i = saved_positions[i].x - saved_positions[parent_index].x;
                saved_direction.j = saved_positions[i].y - saved_positions[parent_index].y;
                saved_direction.k = saved_positions[i].z - saved_positions[parent_index].z;
                current_direction.i = nodes[i].position.x - nodes[parent_index].position.x;
                current_direction.j = nodes[i].position.y - nodes[parent_index].position.y;
                current_direction.k = nodes[i].position.z - nodes[parent_index].position.z;

                vector3d_normalize_with_length(&saved_direction);
                vector3d_normalize_with_length(&current_direction);
                alignment = saved_direction.i * current_direction.i + saved_direction.j * current_direction.j +
                            saved_direction.k * current_direction.k;

                if (alignment - 1.0f >= 0.0001f || alignment - 1.0f <= -0.0001f) {
                    double angle = FUN_00628140();
                    if ((angle >= 9.999999747378752e-05 || angle <= -9.999999747378752e-05) &&
                        (angle < 0.7853981852531433 && angle > -0.7853981852531433)) {
                        float angle_sin;
                        // 0x558be8..0x558c1e and 0x558cb5..0x558ce9: both checks are on the parent's
                        // basis (nodes[*(int16 *)ebp], ebp = &graph_nodes[i].parent_node_index)
                        if (!real_matrix4x3_rotation_is_orthonormal(&nodes[parent_index].forward,
                                &nodes[parent_index].left, &nodes[parent_index].up)) {
                            real_matrix4x3_rotation_rebuild_orthonormal(&nodes[parent_index].forward,
                                &nodes[parent_index].left, &nodes[parent_index].up);
                        }
                        angle_sin = (float)fsin(angle);
                        vector3d_rotate_about_axis(angle_sin, alignment);
                        vector3d_rotate_about_axis(angle_sin, 0.0f); // UNSURE: extraout_EDX, second rotated vector
                        vector3d_normalize_with_length(&saved_direction);
                        vector3d_normalize_with_length(&current_direction);
                        vector3d_cross_product(&rotation_axis, &nodes[parent_index].up, 0);
                        vector3d_normalize_with_length(&rotation_axis);
                        if (!real_matrix4x3_rotation_is_orthonormal(&nodes[parent_index].forward,
                                &nodes[parent_index].left, &nodes[parent_index].up)) {
                            real_matrix4x3_rotation_rebuild_orthonormal(&nodes[parent_index].forward,
                                &nodes[parent_index].left, &nodes[parent_index].up);
                        }
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x558a20):

void FUN_00558a20(int param_1,int param_2)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
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
  int iVar14;
  int iVar15;
  char cVar16;
  uint in_EAX;
  int iVar17;
  undefined4 extraout_EDX;
  float *pfVar18;
  float *pfVar19;
  float10 fVar20;
  int local_38;
  int local_34;

  iVar14 = *(int *)((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                                    (in_EAX & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14
                                       + DAT_0087bc14) + 0x44) & 0xffff) * 0x20 + 0x14 +
                   DAT_0087bc14);
  local_38 = 0;
  if (0 < *(int *)(iVar14 + 0x68)) {
    pfVar18 = (float *)(param_2 + 8);
    pfVar19 = (float *)(param_1 + 0x30);
    local_34 = 0;
    do {
      if (local_34 != 0) {
        iVar15 = *(int *)(iVar14 + 0x6c);
        iVar17 = (int)*(short *)(iVar15 + 0x24 + local_34);
        if ((*(byte *)(iVar17 * 0x40 + 0x28 + iVar15) & 4) == 0) {
          fVar2 = pfVar18[-2];
          fVar3 = *(float *)(param_2 + iVar17 * 0xc);
          fVar4 = pfVar18[-1];
          fVar5 = *(float *)(param_2 + 4 + iVar17 * 0xc);
          fVar6 = *pfVar18;
          fVar7 = *(float *)(param_2 + iVar17 * 0xc + 8);
          pfVar1 = (float *)(iVar17 * 0x34 + 0x28 + param_1);
          fVar8 = pfVar19[-2];
          fVar9 = *pfVar1;
          fVar10 = pfVar19[-1];
          fVar11 = pfVar1[1];
          fVar12 = *pfVar19;
          fVar13 = pfVar1[2];
          vector3d_normalize_with_length();
          vector3d_normalize_with_length();
          vector3d_normalize_with_length();
          fVar2 = (fVar8 - fVar9) * (fVar2 - fVar3) +
                  (fVar10 - fVar11) * (fVar4 - fVar5) + (fVar12 - fVar13) * (fVar6 - fVar7);
          if (0.0001 <= ABS(fVar2 - 1.0)) {
            fVar20 = (float10)FUN_00628140();
            if (((float10)9.999999747378752e-05 <= ABS(fVar20)) &&
               (ABS(fVar20) < (float10)0.7853981852531433)) {
              cVar16 = FUN_005579e0();
              if (cVar16 == '\0') {
                FUN_00558860();
              }
              fVar20 = (float10)fsin((float10)(float)fVar20);
              vector3d_rotate_about_axis((float)fVar20,fVar2);
              vector3d_rotate_about_axis((float)fVar20,extraout_EDX);
              vector3d_normalize_with_length();
              vector3d_normalize_with_length();
              vector3d_cross_product(*(short *)(iVar15 + 0x24 + local_34) * 0x34 + param_1 + 0x1c);
              vector3d_normalize_with_length();
              cVar16 = FUN_005579e0();
              if (cVar16 == '\0') {
                FUN_00558860();
              }
            }
          }
        }
      }
      local_38 = local_38 + 1;
      local_34 = local_34 + 0x40;
      pfVar18 = pfVar18 + 3;
      pfVar19 = pfVar19 + 0xd;
    } while (local_38 < *(int *)(iVar14 + 0x68));
  }
  return;
}
#endif
