// model_nodes_build_matrices  (Ghidra: FUN_004d7690, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table, which also
// corrects the phase 2 summary: "This is [animation_graph_nodes_build_matrices] over GBXModel
// nodes (stride 0x9c) with the root position in EAX, and there is no override.")
// address 0x4d7690, size 280 bytes
// name confidence: 0.5   rewrite confidence: 0.8 (review pass: checked against objdump)
// evidence: same algorithm as animation_graph_nodes_build_matrices (0x4d6880), over GBXModel's
//   own ModelNode array instead of the animation graph's nodes. VERIFIED against objdump -d -M
//   intel bin/halo.exe (scratchpad/halo_disasm.txt, 0x4d7690..0x4d77b1), which -- because this
//   function uses a standard EBP frame -- reads cleanly: EAX (root_position) and ECX (forward)
//   are register arguments the Ghidra decompile drops entirely; [ebp+0x8]/[ebp+0xc]/[ebp+0x10]/
//   [ebp+0x14] are model / out_matrices / orientations / up, in that order (again more stack
//   parameters than the abbreviated "GBXModel*, real_matrix4x3 *out, real_orientation*" in the
//   models_types_notes.md register table -- see the same note on animation_graph_nodes_build_
//   matrices.c).
// register convention: root position in EAX (in_EAX), forward vector in ECX (in_ECX, dropped
//   by Ghidra); model, output node matrices, input node orientations and the up vector as the
//   recognized stack parameters, in that order.
//   // blam-cc: EAX -> root_position, ECX -> forward, stack -> model, out_matrices,
//   //           orientations, up

#include "tags.h"
#include "math.h"
#include "models.h"
#include "fn_math.h"

extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x00696664

extern void matrix4x3_from_quaternion(real_quaternion *q, real_matrix4x3 *out); // 0x4cbad0

// Breadth-first walk of a model's own node tree (as opposed to animation_graph_nodes_build_
// matrices, which walks the animation graph's separate node list), converting each node's
// local SQT orientation into a world-space real_matrix4x3 by composing it with its parent's
// matrix (node 0's "parent" is a virtual matrix built from root_position and the caller-
// supplied forward/up axes).
void model_nodes_build_matrices(real_point3d *root_position, real_vector3d *forward, GBXModel *model,
                                 real_matrix4x3 *out_matrices, real_orientation *orientations, real_vector3d *up)
{
    real_matrix4x3 root_parent;
    int16_t queue[k_maximum_nodes_per_model];
    int16_t read_index, write_index;

    matrix4x3_from_forward_up(up, forward, &root_parent);
    root_parent.position = *root_position;

    if (model->nodes.count <= 0) {
        return;
    }

    read_index = 0;
    write_index = 1;
    queue[0] = 0;
    do {
        ModelNode *node_def;
        real_matrix4x3 *parent_matrix;
        real_matrix4x3 local_matrix;
        int16_t node;

        node = queue[read_index];
        read_index = read_index + 1;
        node_def = (ModelNode *)((uint8_t *)model->nodes.pointer + node * sizeof(ModelNode));

        parent_matrix = (node == 0) ? &root_parent : &out_matrices[(int16_t)node_def->parent_node_index]; // movsx +0x24

        matrix4x3_from_quaternion(&orientations[node].rotation, &local_matrix);
        local_matrix.scale = orientations[node].scale;
        local_matrix.position = orientations[node].translation;

        matrix4x3_multiply_procedure(parent_matrix, &local_matrix, &out_matrices[node]);

        if (node_def->next_sibling_node_index != 0xffff) {
            queue[write_index] = (int16_t)node_def->next_sibling_node_index;
            write_index = write_index + 1;
        }
        if (node_def->first_child_node_index != 0xffff) {
            queue[write_index] = (int16_t)node_def->first_child_node_index;
            write_index = write_index + 1;
        }
    } while (read_index != write_index);
}

#if 0
Original Ghidra decompilation (0x4d7690):

void FUN_004d7690(int param_1,int param_2)

{
  short sVar1;
  undefined4 *in_EAX;
  int extraout_ECX;
  undefined1 *puVar2;
  int iVar3;
  short local_fc [64];
  undefined1 local_7c [40];
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_44 [10];
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  int local_c;
  int local_8;

  matrix4x3_from_forward_up(local_7c);
  local_50 = in_EAX[1];
  local_54 = *in_EAX;
  local_4c = in_EAX[2];
  local_c = 0;
  if (0 < *(int *)(param_1 + 0xb8)) {
    local_8 = 1;
    local_fc[0] = 0;
    do {
      sVar1 = local_fc[(short)local_c];
      local_c = local_c + 1;
      iVar3 = sVar1 * 0x9c + *(int *)(param_1 + 0xbc);
      if (sVar1 == 0) {
        puVar2 = local_7c;
      }
      else {
        puVar2 = (undefined1 *)(*(short *)(iVar3 + 0x24) * 0x34 + param_2);
      }
      matrix4x3_from_quaternion();
      local_44[0] = *(undefined4 *)(extraout_ECX + 0x1c);
      local_1c = *(undefined4 *)(extraout_ECX + 0x10);
      local_18 = *(undefined4 *)(extraout_ECX + 0x14);
      local_14 = *(undefined4 *)(extraout_ECX + 0x18);
      (*(code *)PTR_matrix4x3_multiply_00696664)(puVar2,local_44,sVar1 * 0x34 + param_2);
      if (*(short *)(iVar3 + 0x20) != -1) {
        local_fc[(short)local_8] = *(short *)(iVar3 + 0x20);
        local_8 = local_8 + 1;
      }
      if (*(short *)(iVar3 + 0x22) != -1) {
        local_fc[(short)local_8] = *(short *)(iVar3 + 0x22);
        local_8 = local_8 + 1;
      }
    } while ((short)local_c != (short)local_8);
  }
  return;
}
#endif
