// object_solve_two_bone_ik_to_marker  (Ghidra: FUN_004f6d60; renamed, Blam-style, not
// previously named)
// address 0x4f6d60, size 265 bytes
// name confidence: 0.25 (zero recorded callers -- functions.md flags this as a two-bone IK
//   setup: resolves two markers, walks their nodes' parent chain two levels, and hands the
//   result to model_ik_solve_two_bone; the specific caller/use site is not recovered)
// rewrite confidence: 0.3
// evidence: types/tags.h Object.model, GBXModel.nodes (TagReflexive, pointer at +0xbc,
//   confirmed elsewhere in this batch at 0x4f6b70/0x4f6c60 reading GBXModel+0xb8 as the node
//   count); global 0x008603b0 object_data, global 0x0087bc14 tag_instances, global 0x00696664
//   matrix4x3_multiply_procedure; callees object_get_node_local_transform (0x4f6080, OUTSIDE
//   this batch), matrix4x3_inverse (0x4cb7a0), matrix4x3_multiply (0x4cc0d0),
//   model_ik_solve_two_bone (0x4d6440).
// register convention: object index in ECX; four further values are plain STACK parameters
//   (Ghidra's param_1/param_2/param_3/param_4: a marker name, then a second object index and
//   marker name pair reused verbatim for the second get_node_local_transform call, then a node
//   array base pointer). Confirmed against objdump -d -M intel bin/halo.exe 0x4f6d60..: the
//   entry reads ecx directly (no stack fetch for the object index) while every other input
//   comes from [ebp+0x8]/[ebp+0xc]/[ebp+0x10]/[ebp+0x14].
//   // blam-cc: ECX -> object_index, stack -> marker_a_name, param_2, param_3, node_base
// UNSURE: this file calls object_get_node_local_transform (0x4f6080) exactly as this function's
//   own disassembly passes its arguments -- ALL FOUR on the stack, object index included. That
//   contradicts the EAX/ECX/EDX register convention claimed in that function's own file header
//   (src/objects/object_get_node_local_transform.c), which was written outside this batch and
//   is not touched here. The call sites below follow the plain stack reading, since 0x4f6d80's
//   own disassembly (`mov eax,[esp+0x4]`) confirms the object index is read from the stack at
//   that function's true entry point.
// UNSURE: the ModelNode field this walks at +0x24 (used twice, to climb from the resolved
//   marker's node up two levels) has no established name in this module; ModelNode is not
//   otherwise touched here. The whole function is effectively unreachable dead code (zero
//   callers), so this rewrite favours a faithful direct translation over full field recovery.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x00696664

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080, see UNSURE above about its true ABI
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0
extern void model_ik_solve_two_bone(real_matrix4x3 *target, uint8_t *bone_c, uint8_t *bone_b,
    uint8_t *bone_a); // 0x4d6440, UNSURE: parameter names guessed from the call order below

void object_solve_two_bone_ik_to_marker(uint32_t object_index, char *marker_a_name,
    uint32_t param_2, char *param_3, uint8_t *node_base)
    // blam-cc: ECX -> object_index, stack -> marker_a_name, param_2, param_3, node_base
    // (param_4 in Ghidra's own signature is renamed node_base; param_1 is marker_a_name)
{
    Object *definition = (Object *)tag_instances[
        ((object_header *)object_data->data)[object_index & 0xffff].data->definition_tag & 0xffff].data;
    GBXModel *model = (GBXModel *)tag_instances[definition->model.tag_id.index & 0xffff].data;
    uint8_t *nodes = (uint8_t *)model->nodes.pointer;

    object_marker marker_a;
    object_marker marker_b;

    if (object_get_node_local_transform(object_index, marker_a_name, &marker_a, 1) == 0) {
        return;
    }
    // UNSURE: preserved exactly -- the second call passes this function's own param_2 and
    // param_3 as the object index and marker name, NOT the object index used above. This
    // matches the compiled code, which calls FUN_004f6080(param_2,param_3,local_78,1) verbatim.
    if (object_get_node_local_transform(param_2, param_3, &marker_b, 1) == 0) {
        return;
    }

    {
        int16_t node_b = *(int16_t *)(nodes + marker_a.node_index * 0x9c + 0x24);
        if (node_b != -1) {
            int16_t node_c = *(int16_t *)(nodes + node_b * 0x9c + 0x24);
            if (node_c != -1) {
                real_matrix4x3 inverse;
                real_matrix4x3 combined;

                matrix4x3_inverse(&inverse, &marker_a.transform);
                matrix4x3_multiply_procedure(&marker_b.transform, &inverse, &combined);

                model_ik_solve_two_bone(&combined,
                    node_base + node_c * 0x34,
                    node_base + node_b * 0x34,
                    node_base + marker_a.node_index * 0x34);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f6d60):

void FUN_004f6d60(undefined4 param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  short sVar1;
  int iVar2;
  short sVar3;
  uint in_ECX;
  undefined1 local_120 [56];
  short local_e8;
  undefined1 local_78 [56];
  undefined1 local_40 [60];

  iVar2 = *(int *)((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                                   (in_ECX & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14
                                      + DAT_0087bc14) + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14
                  );
  sVar3 = FUN_004f6080();
  if (sVar3 != 0) {
    sVar3 = FUN_004f6080(param_2,param_3,local_78,1);
    if (sVar3 != 0) {
      iVar2 = *(int *)(iVar2 + 0xbc);
      sVar3 = *(short *)(local_e8 * 0x9c + 0x24 + iVar2);
      if (sVar3 != -1) {
        sVar1 = *(short *)(sVar3 * 0x9c + 0x24 + iVar2);
        if (sVar1 != -1) {
          matrix4x3_inverse();
          (*(code *)PTR_matrix4x3_multiply_00696664)(local_40,local_120,local_120);
          model_ik_solve_two_bone
                    (local_120,sVar1 * 0x34 + param_4,sVar3 * 0x34 + param_4,
                     local_e8 * 0x34 + param_4);
        }
      }
    }
  }
  return;
}
#endif
