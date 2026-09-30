// unit_detach_reposition_and_nudge  (Ghidra: FUN_0056ca40)
// address 0x56ca40, size 459 bytes, name confidence 0.2, rewrite confidence 0.9 (REWRITTEN from objdump)
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
#include "fn_math.h"
#include "fn_objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_get_position(real_point3d *out, uint32_t object_index);          // 0x4f6900, EAX, ECX

extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);         // 0x4f6610, stack
extern uint8_t scenario_structure_bsp_locate_point_nudge_up(real_point3d *point);   // 0x53e870, EDX
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index);          // 0x4f5de0, EAX


extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
    int32_t invoke_callback); // 0x4f9a20, EAX, stack

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

// REWRITTEN from objdump 0x56ca40..0x56cc0a. EDI: unit. A unit with a parent is pushed away from it: the direction
//   from the parent's position to the unit's (else the unit's forward) times 0.02 is added to its velocity after it
//   is detached (0x4f6610), its position nudged up onto the structure (0x53e870 on a copy) and relinked; it drops
//   unit +0x204 bit 15 and object bit 5, sets +0x474, and its lights are reattached. The draft read the unit's own
//   position twice (no push), handed the unit index to the point nudge as a pointer and discarded the nudge.
void unit_detach_reposition_and_nudge(uint32_t unit_index) // blam-cc: EDI -> unit_index
{
    uint8_t *self = OBJECT_DATA(unit_index);                                       // esi
    real_point3d parent_position;                                                   // [esp+0x28]
    real_point3d position;                                                          // [esp+0x1c]
    real_vector3d push;                                                             // [esp+0x10]
    uint8_t *object;
    uint8_t *tag;

    if (((unit_object *)self)->base.parent_object == k_datum_index_none) {
        return;
    }
    object_get_position(&parent_position, ((unit_object *)self)->base.parent_object);
    object_get_position(&position, unit_index);
    push.i = position.x - parent_position.x;
    push.j = position.y - parent_position.y;
    push.k = position.z - parent_position.z;
    if (vector3d_normalize_with_length(&push) == 0.0f) {
        push = *(real_vector3d *)&((unit_object *)self)->base.forward.i;
    }
    push.i = push.i * 0.02f;
    push.j = push.j * 0.02f;
    push.k = push.k * 0.02f;
    object_snap_to_parent_marker_and_detach(unit_index);
    position = *(real_point3d *)&((unit_object *)self)->base.position.x;
    scenario_structure_bsp_locate_point_nudge_up(&position);
    object = OBJECT_DATA(unit_index);
    object_unlink_cluster_or_notify_parent(unit_index);
    *(real_point3d *)(object + 0x5c) = position;
    object_recalculate_bounding_radius(unit_index);
    object_set_cluster_and_parent(unit_index, 0);
    ((unit_object *)self)->unit.flags &= 0xffff7fffu;
    ((unit_object *)self)->base.flags &= 0xffffffdfu;
    self[0x474] = 1;
    ((unit_object *)self)->base.velocity.i = push.i + ((unit_object *)self)->base.velocity.i;
    ((unit_object *)self)->base.velocity.j = push.j + ((unit_object *)self)->base.velocity.j;
    ((unit_object *)self)->base.velocity.k = push.k + ((unit_object *)self)->base.velocity.k;
    object = OBJECT_DATA(unit_index);
    tag = (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
    if (*(int32_t *)(tag + 0x34) != -1 && (object[0x10] & 1)) {
        object_for_each_light_attachment(unit_index, 0, 1);
    }
    if (*(int32_t *)(tag + 0x34) != -1) {
        *(uint32_t *)(object + 0x10) &= ~1u;
        ((object_header *)object_data->data)[unit_index & 0xffff].flags |= 0x02;
    }
    object_recalculate_bounding_radius(unit_index);
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
