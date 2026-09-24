// objects_recompute_cluster_membership  (named by out/phase4/objects_types_notes.md: "0x098
// leaf, 0x09c cluster | object_set_cluster_and_parent 0x4f5c30,
// objects_recompute_cluster_membership 0x4f7570")
// address 0x4f7570, size 353 bytes
// name confidence: 0.85 (fixed by the types notes' own citation of this address by this name)
// rewrite confidence: 0.45 (raised from 0.3 by the phase-4 review pass: the leaf/cluster
// lookup was corrected against the disassembly and the location pair is now typed
// bsp_leaf_reference. Single caller, heavy foreign-module dependency; the exact argument
//   split of bsp3d_node_find_leaf's 64-bit return and collision_bsp_query_sphere_init's three arguments could not be
//   pinned down by hand-tracing the disassembly in the time available -- see UNSURE below)
// evidence: types/objects.h object (flags 0x10 with _object_needs_cluster_update_bit,
//   parent_object 0x11c, location_cluster_index 0x09c, bounding_radius 0x0ac), object_header
//   (cluster_index 0x04); global 0x008603b0 object_data; callees object_iterator_next
//   (0x4f6f20, this batch), object_set_cluster_and_parent (0x4f5c30, established: EAX ->
//   object_index, ECX -> location), bsp3d_node_find_leaf (0x5013a0, same "leaf/visibility probe"
//   already declared by src/objects/object_set_cluster_and_parent.c), collision_bsp_query_sphere_init (0x501980,
//   foreign module, not otherwise seen in this batch).
// register convention: no parameters (matches functions.md: a bulk per-tick sweep).
// UNSURE: collision_bsp_query_sphere_init's three arguments (0, the high dword of bsp3d_node_find_leaf's 64-bit result,
//   and the object's bounding_radius) and bsp3d_node_find_leaf's own apparent 64-bit return here
//   (versus the plain int32_t leaf index it returns at its other, established call site) are
//   preserved as raw EAX/EDX halves rather than resolved to named fields.
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern uint8_t *global_structure_bsp; // 0x00746f9c
extern uint32_t global_structure_collision_bsp; // 0x00746f98, UNSURE: foreign module

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, this batch
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30
extern int32_t bsp3d_node_find_leaf(void *globals, real_point3d *point, int32_t index); // 0x5013a0, UNSURE: see file header
extern void collision_bsp_query_sphere_init(uint32_t zero, uint32_t probe_high_dword, float radius,
    int32_t *out_leaf, int32_t *out_leaf_valid); // 0x501980, foreign module, UNSURE: the two
    // out-params are a stack buffer (lea esi,[esp+0x34] at the call site) this rewrite could
    // not otherwise place; out_leaf_valid gates whether *out_leaf is used directly or
    // bsp3d_node_find_leaf is called again

void objects_recompute_cluster_membership(void)
{
    object_iterator iterator;
    object *obj;

    iterator.type_mask = 0xffffffff;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        if (((obj->flags & _object_needs_cluster_update_bit) != 0) &&
            (obj->parent_object == k_datum_index_none)) {
            object_header *header = (object_header *)object_data->data + (iterator.handle & 0xffff);
            uint64_t probe;
            int32_t leaf;
            int16_t cluster;
            bsp_leaf_reference location;

            obj->flags &= ~(uint32_t)_object_needs_cluster_update_bit;
            obj->location_cluster_index = -1;
            header->cluster_index = -1;

            probe = bsp3d_node_find_leaf(global_collision_bsp, &obj->bounding_center, 0);
            // PHASE-4 REVIEW: this lookup previously read
            // `*(int16_t *)(global_structure_bsp + 0xe4 + leaf * 0x10 + 8)`, which is two
            // bugs at once -- global_structure_bsp+0xe4 holds a POINTER to the
            // ScenarioStructureBSPLeaf array, and the leaf index is masked with 0x7fffffff
            // before it is scaled. objdump confirms both (`mov ecx,[ebx+0xe4]` /
            // `and eax,0x7fffffff` / `shl eax,4` / `movsx eax,WORD PTR [eax+ecx+0x8]`).
            leaf = (int32_t)probe;
            cluster = (leaf == -1) ? -1 :
                *(int16_t *)(*(uint8_t **)(global_structure_bsp + 0xe4) +
                             (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);

            if (cluster == -1) {
                int32_t probed_leaf;
                int32_t probed_leaf_valid;
                collision_bsp_query_sphere_init(0, (uint32_t)(probe >> 32), obj->bounding_radius, &probed_leaf, &probed_leaf_valid);
                leaf = (probed_leaf_valid == 0) ? (int32_t)bsp3d_node_find_leaf(global_collision_bsp, &obj->bounding_center, 0) : probed_leaf;
                cluster = (leaf == -1) ? -1 :
                *(int16_t *)(*(uint8_t **)(global_structure_bsp + 0xe4) +
                             (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);
            }

            location.leaf_index = leaf;
            location.cluster_index = cluster;
            object_set_cluster_and_parent(iterator.handle, &location);
        }
        obj = object_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x4f7570):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004f7570(void)

{
  int iVar1;
  int iVar2;
  undefined8 uVar3;
  int local_1028;
  short local_1024;
  undefined4 local_1020;
  undefined1 local_101c;
  undefined2 local_101a;
  uint local_1018;
  undefined4 local_1014;
  int local_404;
  int local_400;
  undefined4 uStack_4;

  uStack_4 = 0x4f757a;
  local_1014 = 0x86868686;
  local_1020 = 0xffffffff;
  local_101c = 0;
  local_101a = 0;
  local_1018 = 0xffffffff;
  iVar2 = object_iterator_next(&local_1020);
  iVar1 = DAT_008603b0;
  while (iVar2 != 0) {
    DAT_008603b0 = iVar1;
    if (((*(uint *)(iVar2 + 0x10) & 0x800) != 0) && (*(int *)(iVar2 + 0x11c) == -1)) {
      *(uint *)(iVar2 + 0x10) = *(uint *)(iVar2 + 0x10) & 0xfffff7ff;
      *(undefined2 *)(iVar2 + 0x9c) = 0xffff;
      *(undefined2 *)(*(int *)(iVar1 + 0x34) + 4 + (local_1018 & 0xffff) * 0xc) = 0xffff;
      uVar3 = FUN_005013a0();
      iVar1 = DAT_00746f9c;
      local_1028 = (int)uVar3;
      if ((local_1028 == -1) ||
         (local_1024 = *(short *)(local_1028 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4)),
         local_1024 == -1)) {
        FUN_00501980(0,(int)((ulonglong)uVar3 >> 0x20),*(undefined4 *)(iVar2 + 0xac));
        iVar2 = local_400;
        if (local_404 == 0) {
          iVar2 = FUN_005013a0();
        }
        local_1028 = iVar2;
        if (iVar2 == -1) {
          local_1024 = -1;
        }
        else {
          local_1024 = *(short *)(iVar2 * 0x10 + 8 + *(int *)(iVar1 + 0xe4));
        }
      }
      FUN_004f5c30(local_1018,&local_1028);
    }
    iVar2 = object_iterator_next(&local_1020);
    iVar1 = DAT_008603b0;
  }
  DAT_008603b0 = iVar1;
  return;
}
#endif
