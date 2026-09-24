// unit_detach_reposition_and_nudge  (Ghidra: FUN_0056ca40)
// address 0x56ca40, size 459 bytes, name confidence 0.2, rewrite confidence 0.2
// functions.md's summary ("Smoothly steers the unit's stored aim/look direction toward a target
// position each tick") does not match this function's actual field accesses -- it operates on
// object.position (0x5c) and object.velocity (0x68), not unit_data.aiming_vector (0x23c) or
// .looking_vector (0x260). This rewrite follows the literal arithmetic instead: it clears the
// unit's parent link, snapshots and restores its position around a reposition helper call, nudges
// its velocity by a normalized direction, and clears two flag bits plus sets unit_data.unknown_474.
// evidence: types/objects.h object.position (0x5c), object.velocity (0x68), object.flags (0x10),
//   object.parent_object (0x11c); types/units.h unit_data.unknown_474 (0x474).
// blam-cc: unaff_EDI -> unit_index.
// UNSURE: the second object_get_position call's target object is not visible in this
// decompilation (only the unit's own index is); scenario_structure_bsp_locate_point_nudge_up's purpose and why its output is
// discarded (the position is saved before it runs and restored unconditionally after) are not
// recovered.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_get_position(real_point3d *out, uint32_t object_index);          // 0x4f6900
extern real vector3d_normalize_with_length(real_vector3d *v);                       // 0x401990
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);                                      // 0x4f6610, UNSURE signature
extern void scenario_structure_bsp_locate_point_nudge_up(uint32_t unit_index);                                      // 0x53e870, UNSURE signature
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index);          // 0x4f5de0, UNSURE signature
extern void object_recalculate_bounding_radius(uint32_t object_index);              // 0x4f8310, UNSURE signature
extern void object_set_cluster_and_parent(uint32_t object_index);                   // 0x4f5c30, UNSURE signature  // real signature (object_set_cluster_and_parent.c): void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); Ghidra recovered 1 of 2 args at this call site
extern void object_for_each_light_attachment(uint32_t object_index, uint32_t flag); // 0x4f9a20, UNSURE signature  // real signature (object_for_each_light_attachment.c): void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); Ghidra recovered 2 of 3 args at this call site

void unit_detach_reposition_and_nudge(uint32_t unit_index) // blam-cc: unaff_EDI
{
    object *self_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    if (self_obj->parent_object == k_datum_index_none) {
        return;
    }

    real_point3d a, b;
    object_get_position(&a, unit_index);
    object_get_position(&b, unit_index); // UNSURE: second target object not recovered
    real_vector3d dir = { b.x - a.x, b.y - a.y, b.z - a.z };
    float len = vector3d_normalize_with_length(&dir);
    if (len == 0.0f) {
        dir = self_obj->forward;
    }

    object_snap_to_parent_marker_and_detach(unit_index);
    real_point3d saved_position = self_obj->position;
    scenario_structure_bsp_locate_point_nudge_up(unit_index);
    self_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    object_unlink_cluster_or_notify_parent(unit_index);
    self_obj->position = saved_position;
    object_recalculate_bounding_radius(unit_index);
    object_set_cluster_and_parent(unit_index);

    unit_data *unit = (unit_data *)((uint8_t *)self_obj + k_unit_data_offset);
    unit->flags &= 0xffff7fff;
    self_obj->flags &= 0xffffffdf;
    unit->unknown_474 = 1;
    self_obj->velocity.i = dir.i * 0.020000001f + self_obj->velocity.i;
    self_obj->velocity.j = dir.j * 0.020000001f + self_obj->velocity.j;
    self_obj->velocity.k = dir.k * 0.020000001f + self_obj->velocity.k;

    // NOTE: original re-fetches the object here through the header/tag lookup a second time;
    // we already hold `self_obj`, so the tag lookup below reuses it directly.
    Object *self_def = (Object *)tag_instances[self_obj->definition_tag & 0xffff].data;
    if ((*(uint32_t *)&self_def->model.tag_id != 0xffffffff) && ((self_obj->flags & 1) != 0)) {
        object_for_each_light_attachment(unit_index, 1);
    }
    if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
        self_obj->flags &= ~1u;                          // object + 0x10, bit 0
        ((object_header *)object_data->data)[unit_index & 0xffff].flags |= 0x02;
    }
    object_recalculate_bounding_radius(unit_index);
    return;
}

#if 0
Original Ghidra decompilation (0x56ca40):

void FUN_0056ca40(void)

{
  byte *pbVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  int iVar6;
  uint *puVar7;
  int iVar8;
  uint unaff_EDI;
  float10 fVar9;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  iVar8 = (unaff_EDI & 0xffff) * 0xc;
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
  if (*(int *)(iVar2 + 0x11c) != -1) {
    object_get_position();
    object_get_position();
    local_24 = local_18 - local_c;
    local_20 = local_14 - local_8;
    local_1c = local_10 - local_4;
    fVar9 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar9) {
      local_24 = *(float *)(iVar2 + 0x74);
      local_20 = *(float *)(iVar2 + 0x78);
      local_1c = *(float *)(iVar2 + 0x7c);
    }
    FUN_004f6610();
    uVar3 = *(undefined4 *)(iVar2 + 0x5c);
    uVar4 = *(undefined4 *)(iVar2 + 0x60);
    uVar5 = *(undefined4 *)(iVar2 + 100);
    FUN_0053e870();
    iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
    object_unlink_cluster_or_notify_parent();
    *(undefined4 *)(iVar6 + 0x5c) = uVar3;
    *(undefined4 *)(iVar6 + 0x60) = uVar4;
    *(undefined4 *)(iVar6 + 100) = uVar5;
    object_recalculate_bounding_radius();
    object_set_cluster_and_parent();
    *(uint *)(iVar2 + 0x204) = *(uint *)(iVar2 + 0x204) & 0xffff7fff;
    iVar6 = DAT_008603b0;
    *(uint *)(iVar2 + 0x10) = *(uint *)(iVar2 + 0x10) & 0xffffffdf;
    *(undefined1 *)(iVar2 + 0x474) = 1;
    *(float *)(iVar2 + 0x68) = local_24 * 0.020000001 + *(float *)(iVar2 + 0x68);
    *(float *)(iVar2 + 0x6c) = local_20 * 0.020000001 + *(float *)(iVar2 + 0x6c);
    *(float *)(iVar2 + 0x70) = local_1c * 0.020000001 + *(float *)(iVar2 + 0x70);
    puVar7 = *(uint **)(*(int *)(iVar6 + 0x34) + 8 + iVar8);
    iVar2 = *(int *)((*puVar7 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if ((*(int *)(iVar2 + 0x34) != -1) && ((puVar7[4] & 1) != 0)) {
      object_for_each_light_attachment(0,1);
    }
    if (*(int *)(iVar2 + 0x34) != -1) {
      iVar2 = *(int *)(DAT_008603b0 + 0x34);
      puVar7[4] = puVar7[4] & 0xfffffffe;
      pbVar1 = (byte *)(iVar2 + iVar8 + 2);
      *pbVar1 = *pbVar1 | 2;
    }
    object_recalculate_bounding_radius();
  }
  return;
}
#endif
