// object_set_cluster_and_parent
// address 0x4f5c30, size 432 bytes
// name confidence: 0.4 (still FUN_004f5c30 in Ghidra; functions.md's summary matches the code:
//   "Assigns an object's containing cluster (if unparented) or links it into its parent's child
//   chain, then updates cluster-membership bookkeeping and may trigger deletion of stale
//   sub-objects")
// rewrite confidence: 0.9 (VERIFIED against objdump) (raised from 0.4 by the phase-4 review pass: the leaf/cluster lookup was corrected and the location pair is now typed bsp_leaf_reference)
// evidence: types/objects.h object_header (flags at 0x02 with in_pvs_pass/unknown_80 bits),
//   object (parent_object 0x11c, next_object 0x114, first_child_object 0x118,
//   location_leaf_index 0x098, location_cluster_index 0x09c, placement_id 0x10c,
//   bounding_center 0x0a0, bounding_radius 0x0ac, flags 0x10 with
//   _object_outside_map_bit/_object_needs_cluster_update_bit/_object_connected_to_map_bit);
//   global 0x008603b0 object_data; global 0x0087a478 (the same BSP PVS source used in
//   objects_update, this batch); callees object_mark_pending_delete (0x4f50f0, this batch) and
//   object_delete (0x4f5bd0, this batch).
// FIXED (objdump 0x4f5c30): both arguments are on the stack -- the object index ([esp+4]) and an optional
//   {leaf_index, cluster_index} location pointer ([esp+0x20] after the frame; NULL to probe the object's own
//   position for it). Neither EAX nor ECX is an input.
// UNSURE: the write of a full 32-bit value at object+0x9c straddles the documented
//   location_cluster_index (int16 at 0x09c) and the undocumented unknown_09e that immediately
//   follows it; preserved exactly as a dword store rather than split into two fields, since
//   types/objects.h explicitly notes 0x09e is never read by anything in this module. UNSURE:
//   FUN_00551f00 and scenario_location_from_point (foreign, outside this module's address range) are called with
//   their literal argument lists, not renamed.
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern player_globals *local_player_globals; // 0x0087a478
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern ScenarioStructureBSP *global_structure_bsp;
                                           //   this is the same DAT_00746f9c base object_update
                                           //   reads at +0x134, here read at +0xe4

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point
extern void scenario_location_from_point(bsp_leaf_reference *out, real_point3d *point); // 0x53e780, ESI out, EDX point
extern datum_index *noncollideable_cluster_first; // 0x008603c0, the per-cluster list descriptor
extern datum_index *collideable_cluster_first;    // 0x008603d0
extern void cluster_reference_add_within_radius(uint32_t light_or_object_handle, datum_index *placement_slot, real_point3d *position,
                          float radius, void *leaf_and_cluster, void *cluster_list);
    // 0x551f00; the first four are stack arguments, the leaf/cluster pair arrives in EAX and the
    // cluster-list descriptor in EDI. Resolved at 0x4f5d50: mov edi,0x8603c0 /
    // lea eax,[ebp+0x98] -- and object+0x98 is exactly object.location_leaf_index followed by
    // object.location_cluster_index, so the pair the helper fills in is stored straight back
    // into the object.
extern void object_mark_pending_delete(uint32_t object_index); // 0x4f50f0, this batch
extern void object_delete(uint32_t object_index); // 0x4f5bd0, this batch

// blam-cc: stack -> object_index, location
void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location)
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;

    if (obj->parent_object == k_datum_index_none) {
        bsp_leaf_reference local_location;

        if (location == 0) {
            int32_t leaf = bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)global_collision_bsp, &obj->bounding_center); // 0x4f5ca2
            if (leaf == -1) {
                local_location.cluster_index = -1;
            } else {
                // PHASE-4 REVIEW: global_structure_bsp+0xe4 holds a POINTER to the
                // ScenarioStructureBSPLeaf array and the leaf index is masked with 0x7fffffff
                // before scaling -- 0x4f5cc7 `mov ecx,[edx+0xe4]` / `and eax,0x7fffffff` /
                // `shl eax,4`. Both steps were missing from the earlier rewrite.
                local_location.cluster_index = *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer +
                                                            (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);
            }
            local_location.leaf_index = leaf;
            location = &local_location;
            if (local_location.cluster_index == -1) {
                // 0x4f5ced: ESI = &local_location, EDX = the object's position (+0x5c)
                scenario_location_from_point(&local_location, (real_point3d *)((uint8_t *)obj + 0x5c));
            }
        }

        if (location->cluster_index == -1) {
            obj->flags |= _object_outside_map_bit;
        } else {
            obj->location_leaf_index = location->leaf_index;
            // The original stores the whole dword at object+0x9c, which covers
            // location_cluster_index and the int16 after it; kept as the 32-bit write.
            *(int32_t *)&((object *)obj)->location_cluster_index = *(int32_t *)&location->cluster_index;
            header->cluster_index = location->cluster_index;
            obj->flags &= ~(uint32_t)_object_outside_map_bit;
        }

        header->flags &= (uint8_t)~_object_header_unknown_80_bit;

        // 0x4f5d42..0x4f5d50: EDI = 0x8603d0 (collideable) when object flag 0x2000000 is set, else 0x8603c0 -- the
        // same choice object_unlink_cluster_or_notify_parent makes. Always using the noncollideable list linked
        // collideable objects into one list and unlinked them from the other: campaign-entry crash in datum_delete.
        cluster_reference_add_within_radius(object_index, &obj->placement_id, &obj->bounding_center, obj->bounding_radius,
                     &obj->location_leaf_index,
                     (obj->flags & 0x2000000) != 0 ? (void *)&collideable_cluster_first : (void *)&noncollideable_cluster_first);

        if ((header->flags & _object_header_in_pvs_pass_bit) != 0) {
            int16_t cluster = header->cluster_index;
            if (cluster == -1 ||
                (*(uint32_t *)&local_player_globals->cluster_pvs[(cluster >> 5)] &
                 (1u << (cluster & 0x1f))) == 0) {
                if ((obj->flags & _object_connected_to_map_bit) != 0) {
                    object_delete(object_index);
                }
            } else {
                object_mark_pending_delete(object_index);
            }
        }
    } else {
        object_header *parent_header =
            (object_header *)object_data->data + (obj->parent_object & 0xffff);
        object *parent = parent_header->data;

        obj->next_object = parent->first_child_object;
        parent->first_child_object = object_index;
        header->flags |= _object_header_unknown_80_bit;
        obj->location_cluster_index = -1;
    }

    obj->flags |= _object_needs_cluster_update_bit;
    header->flags |= _object_header_connected_bit;
}

#if 0
Original Ghidra decompilation (0x4f5c30):

void FUN_004f5c30(uint param_1,int *param_2)

{
  short sVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int local_8;
  short local_4;

  iVar6 = (param_1 & 0xffff) * 0xc;
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  iVar5 = *(int *)(DAT_008603b0 + 0x34) + iVar6;
  if (*(uint *)(iVar2 + 0x11c) == 0xffffffff) {
    if (param_2 == (int *)0x0) {
      local_8 = FUN_005013a0();
      if (local_8 == -1) {
        local_4 = -1;
      }
      else {
        local_4 = *(short *)(local_8 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
      }
      param_2 = &local_8;
      if (local_4 == -1) {
        FUN_0053e780();
      }
    }
    if ((short)param_2[1] == -1) {
      uVar4 = *(uint *)(iVar2 + 0x10) | 0x200000;
    }
    else {
      *(int *)(iVar2 + 0x98) = *param_2;
      iVar6 = param_2[1];
      *(int *)(iVar2 + 0x9c) = iVar6;
      *(short *)(iVar5 + 4) = (short)iVar6;
      uVar4 = *(uint *)(iVar2 + 0x10) & 0xffdfffff;
    }
    *(uint *)(iVar2 + 0x10) = uVar4;
    *(byte *)(iVar5 + 2) = *(byte *)(iVar5 + 2) & 0x7f;
    FUN_00551f00(param_1,iVar2 + 0x10c,iVar2 + 0xa0,*(undefined4 *)(iVar2 + 0xac));
    if ((*(byte *)(iVar5 + 2) & 0x40) != 0) {
      sVar1 = *(short *)(iVar5 + 4);
      if ((sVar1 == -1) ||
         ((*(uint *)(DAT_0087a478 + 0x18 + ((int)sVar1 >> 5) * 4) & 1 << ((byte)sVar1 & 0x1f)) == 0)
         ) {
        if ((*(uint *)(iVar2 + 0x10) & 0x80000) != 0) {
          FUN_004f5bd0();
        }
      }
      else {
        FUN_004f50f0();
      }
    }
  }
  else {
    iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar2 + 0x11c) & 0xffff) * 0xc);
    *(undefined4 *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6) + 0x114) =
         *(undefined4 *)(iVar3 + 0x118);
    *(uint *)(iVar3 + 0x118) = param_1;
    *(byte *)(iVar5 + 2) = *(byte *)(iVar5 + 2) | 0x80;
    *(undefined2 *)(iVar2 + 0x9c) = 0xffff;
  }
  *(uint *)(iVar2 + 0x10) = *(uint *)(iVar2 + 0x10) | 0x800;
  *(byte *)(iVar5 + 2) = *(byte *)(iVar5 + 2) | 0x20;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
