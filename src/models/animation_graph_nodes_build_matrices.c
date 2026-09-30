// animation_graph_nodes_build_matrices  (Ghidra: model_nodes_calculate_world_transforms, wrong
// name; renamed per out/phase4/models_types_notes.md "Misnamed or misattributed functions"
// table -- this walks the ANIMATION GRAPH's nodes (ModelAnimationsAnimationGraphNode, stride
// 0x40), not the model's own ModelNode array; the model_nodes_build_matrices at 0x4d7690 is
// this same breadth-first algorithm run over GBXModel's ModelNode array instead)
// address 0x4d6880, size 327 bytes
// name confidence: 0.5   rewrite confidence: 0.8 (review pass: checked against objdump)
// evidence: out/phase4/models_types_notes.md ModelAnimationsAnimationGraphNode section
//   (breadth-first walk over next_sibling_node_index/first_child_node_index, parent_node_index
//   as the parent matrix index) and real_orientation section (matrix4x3_from_quaternion feeds
//   +0x1c into matrix.scale and +0x10 into matrix.position). VERIFIED against objdump -d -M
//   intel bin/halo.exe (scratchpad/halo_disasm.txt, 0x4d6880..0x4d69d0): the doc's register
//   table lists only "real_matrix4x3 *out, real_orientation*" as stack parameters, but the
//   disassembly shows two more (the up/forward vectors passed straight through to
//   matrix4x3_from_forward_up to build node 0's virtual parent matrix) at true stack offsets
//   0xfc/0x100, alongside out_matrices/orientations at 0xf4/0xf8 -- four stack parameters in
//   total, not two.
//   The per-node local matrix passed as matrix4x3_multiply's 'b' operand is built by calling
//   matrix4x3_from_quaternion on orientations[node].rotation (which also sets scale=1 and
//   zeroes position), then patching just its scale and position fields from
//   orientations[node].scale/.translation -- i.e. the standard SQT-to-matrix expansion that
//   real_orientation exists for.
// register convention: animation graph tag id in EAX (in_EAX), root position in ECX (in_ECX);
//   output node matrices, input node orientations, forward vector and up vector as the
//   recognized stack parameters, in that order.
//   // blam-cc: EAX -> animation_graph_tag, ECX -> root_position, stack -> out_matrices,
//   //           orientations, forward, up

#include "tags.h"
#include "math.h"
#include "cache.h"
#include "models.h"
#include "fn_math.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x00696664

extern void matrix4x3_from_quaternion(real_quaternion *q, real_matrix4x3 *out); // 0x4cbad0

// Breadth-first walk of an animation graph's node tree, converting each node's local SQT
// orientation into a world-space real_matrix4x3 by composing it with its parent's matrix
// (node 0's "parent" is a virtual matrix built from root_position and the caller-supplied
// forward/up axes).
void animation_graph_nodes_build_matrices(datum_index animation_graph_tag, real_point3d *root_position,
                                           real_matrix4x3 *out_matrices, real_orientation *orientations,
                                           real_vector3d *forward, real_vector3d *up)
{
    ModelAnimations *graph;
    real_matrix4x3 root_parent;
    int16_t queue[k_maximum_nodes_per_model];
    int16_t read_index, write_index;

    graph = (ModelAnimations *)tag_instances[animation_graph_tag & 0xffff].data;

    matrix4x3_from_forward_up(up, forward, &root_parent);
    root_parent.position = *root_position;

    if (graph->nodes.count <= 0) {
        return;
    }

    read_index = 0;
    write_index = 1;
    queue[0] = 0;
    do {
        ModelAnimationsAnimationGraphNode *node_def;
        real_matrix4x3 *parent_matrix;
        real_matrix4x3 local_matrix;
        int16_t node;

        node = queue[read_index];
        read_index = read_index + 1;
        node_def = (ModelAnimationsAnimationGraphNode *)((uint8_t *)graph->nodes.pointer + node * sizeof(ModelAnimationsAnimationGraphNode));

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
Original Ghidra decompilation (0x4d6880):

void model_nodes_calculate_world_transforms(int param_1)

{
  short sVar1;
  int iVar2;
  uint in_EAX;
  short sVar3;
  undefined4 *in_ECX;
  int extraout_ECX;
  int iVar4;
  undefined1 *puVar5;
  short sVar6;
  int iVar7;
  short asStackY_10080 [32698];
  undefined4 local_e8 [10];
  undefined4 local_c0;
  undefined4 local_bc;
  undefined4 local_b8;
  undefined1 local_b4 [40];
  undefined4 local_8c;
  undefined4 local_88;
  undefined4 local_84;
  short local_80 [64];

  iVar2 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  matrix4x3_from_forward_up(local_b4);
  local_8c = *in_ECX;
  local_88 = in_ECX[1];
  local_84 = in_ECX[2];
  sVar6 = 0;
  if (0 < *(int *)(iVar2 + 0x68)) {
    sVar3 = 1;
    local_80[0] = 0;
    do {
      sVar1 = local_80[sVar6];
      sVar6 = sVar6 + 1;
      iVar7 = sVar1 * 0x40 + *(int *)(iVar2 + 0x6c);
      if (sVar1 == 0) {
        puVar5 = local_b4;
      }
      else {
        puVar5 = (undefined1 *)(*(short *)(iVar7 + 0x24) * 0x34 + param_1);
      }
      matrix4x3_from_quaternion();
      local_e8[0] = *(undefined4 *)(extraout_ECX + 0x1c);
      local_c0 = *(undefined4 *)(extraout_ECX + 0x10);
      local_bc = *(undefined4 *)(extraout_ECX + 0x14);
      local_b8 = *(undefined4 *)(extraout_ECX + 0x18);
      (*(code *)PTR_matrix4x3_multiply_00696664)(puVar5,local_e8,sVar1 * 0x34 + param_1);
      if (*(short *)(iVar7 + 0x20) != -1) {
        iVar4 = (int)sVar3;
        sVar3 = sVar3 + 1;
        local_80[iVar4] = *(short *)(iVar7 + 0x20);
      }
      if (*(short *)(iVar7 + 0x22) != -1) {
        iVar4 = (int)sVar3;
        sVar3 = sVar3 + 1;
        local_80[iVar4] = *(short *)(iVar7 + 0x22);
      }
    } while (sVar6 != sVar3);
  }
  return;
}

objdump -d -M intel bin/halo.exe, 0x4d6880..0x4d69d0 (key excerpts):
  4d6890  mov ecx,ds:0x87bc14 / mov ebx,[eax+ecx+0x14]     ebx = tag_instances[graph & 0xffff].data
  4d689d  mov eax,[esp+0x10c] / mov ecx,[esp+0x108]         (true offsets 0x100 / 0xfc: up, forward)
  4d68ab  lea edx,[esp+0x48] / push edx / call 0x4cb970     matrix4x3_from_forward_up(up, forward, &root_parent)
  4d68b9  mov eax,[esi] / ecx,[esi+4] / edx,[esi+8] -> root_parent.position = *root_position (esi = saved ECX)
  4d6928  mov eax,[esp+0x104]                                (true 0xf4: out_matrices)
  4d6934  mov eax,[esp+0x108]                                (true 0xf8: orientations)
  4d6942  lea edx,[esp+0x18] / call 0x4cbad0                 matrix4x3_from_quaternion(&orientations[node], &local_matrix)
  4d694e  mov edx,[ecx+0x1c] / mov [esp+0x18],edx            local_matrix.scale = orientations[node].scale
  4d6958..4d6978  translation.x/y/z -> local_matrix.position
  4d697c  call ds:0x696664                                   matrix4x3_multiply_procedure(parent, &local_matrix, &out_matrices[node])
#endif
