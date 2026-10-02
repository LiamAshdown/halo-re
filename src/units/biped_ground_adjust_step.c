// biped_ground_adjust_step  (Ghidra: biped_ground_adjust_step, renamed)
// address 0x557a90, size 235 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: touches biped_data.ground_adjust_iteration / ground_adjust_iteration_limit
//   (object 0x524/0x525, named by types/units.h from 0x55ad00's seeding and this function's
//   own increment), drives the ground-contact solver (0x558000) and the node-basis updater
//   (0x558a20), and snapshots node positions into the module-owned global
//   unit_ground_adjust_node_positions (0x006e4a08, documented in types/units.h "globals this
//   module owns").
// register convention: stack -> object_index only (0x55e883 pushes one argument; 0x557a9e reads it).
//   The EBX that biped_ground_adjust_solve_node reads is set by biped_ground_adjust_solve itself
//   (0x558264, the slide's out position), not threaded down from here.
//   // blam-cc: stack -> object_index
// UNSURE: the returned value packs a throwaway upper 24 bits (CONCAT31 of an unrelated dead
//   value) around the real 1-byte result in the original; only the low byte is ever read by
//   callers, so this rewrite returns just that byte.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;                             // 0x008603b0
extern tag_instance *tag_instances;                          // 0x0087bc14
extern real_point3d unit_ground_adjust_node_positions[64];   // 0x006e4a08

extern void biped_ground_adjust_solve(uint32_t object_index, real_matrix4x3 *nodes); // 0x558000, stack (0x557b48 pushes nodes, object)
extern void biped_ground_adjust_apply_node_rotations(uint32_t object_index, real_matrix4x3 *nodes,
                                                       real_point3d *saved_positions); // 0x558a20

// Snapshots the current world position of every skeleton node into the shared
// unit_ground_adjust_node_positions buffer, runs the ground-contact/bone-length solve
// (biped_ground_adjust_solve) and the node-basis update (biped_ground_adjust_apply_node_rotations),
// then advances the per-biped ground-adjust iteration counter, saturating at 0x7f.
// Returns 1 once biped_data.ground_adjust_iteration has already reached
// ground_adjust_iteration_limit (nothing done this call), 0 if an iteration ran.
uint32_t biped_ground_adjust_step(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size); // biped_data starts at object+0x4cc
    Object *object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    ModelAnimations *graph = (ModelAnimations *)tag_instances[object_tag->animation_graph.tag_id.index].data;
    real_matrix4x3 *nodes = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset);
    uint32_t already_capped = biped->ground_adjust_iteration_limit <= biped->ground_adjust_iteration;

    if (!already_capped) {
        int32_t i;
        for (i = 0; i < (int32_t)graph->nodes.count; i++) {
            unit_ground_adjust_node_positions[i] = nodes[i].position;
        }
        biped_ground_adjust_solve(object_index, nodes);
        biped_ground_adjust_apply_node_rotations(object_index, nodes, unit_ground_adjust_node_positions);
        if (biped->ground_adjust_iteration < 0x7f) {
            biped->ground_adjust_iteration = biped->ground_adjust_iteration + 1;
        }
    }
    return already_capped;
}

#if 0
Original Ghidra decompilation (0x557a90):

undefined4 FUN_00557a90(uint param_1)

{
  byte bVar1;
  uint *puVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  int iVar5;
  undefined4 *puVar7;
  char cVar8;
  int iVar9;
  int local_8;
  undefined3 uVar6;
  
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar5 = *(int *)((*(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x44) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar9 = (int)*(short *)((int)puVar2 + 0x1f2) + (int)puVar2;
  cVar8 = '\x01' - ((byte)puVar2[0x149] < *(byte *)((int)puVar2 + 0x525));
  if (cVar8 == '\0') {
    local_8 = 0;
    if (0 < *(int *)(iVar5 + 0x68)) {
      puVar7 = &DAT_006e4a08;
      puVar3 = (undefined4 *)(iVar9 + 0x28);
      do {
        *puVar7 = *puVar3;
        puVar7[1] = puVar3[1];
        puVar7[2] = puVar3[2];
        local_8 = local_8 + 1;
        puVar3 = puVar3 + 0xd;
        puVar7 = puVar7 + 3;
      } while (local_8 < *(int *)(iVar5 + 0x68));
    }
    FUN_00558000(param_1,iVar9);
    uVar4 = FUN_00558a20(iVar9,&DAT_006e4a08);
    bVar1 = (byte)puVar2[0x149];
    uVar6 = (undefined3)((uint)uVar4 >> 8);
    iVar5 = CONCAT31(uVar6,bVar1);
    if (bVar1 < 0x7f) {
      iVar5 = CONCAT31(uVar6,bVar1 + 1);
      *(byte *)(puVar2 + 0x149) = bVar1 + 1;
    }
  }
  return CONCAT31((int3)((uint)iVar5 >> 8),cVar8);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
