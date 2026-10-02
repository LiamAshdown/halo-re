// unit_try_select_equipment  (Ghidra: FUN_0056d1a0)
// address 0x56d1a0, size 273 bytes, name confidence 0.3, rewrite confidence 0.3
// functions.md: "Switches the unit's currently selected secondary item (e.g. grenade type) to
// param_2 if none is already selected, releasing the previous selection when requested."
// evidence: types/units.h unit_data.equipment_object_index (0x318).
// blam-cc: param_1 -> unit_index, param_2 -> new_equipment_object_index,
//   param_3 -> release_current.
// UNSURE: player_index_from_unit_index/equipment_pickup_play_sound/item_set_holder's real roles are not recovered (same pattern
// as unit_try_give_grenade.c and unit_set_or_test_seat_and_weapon_label's neighbors).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *player_data;     // 0x0087a480

extern void object_delete(uint32_t object_index);                                  // 0x4f5bd0, UNSURE signature
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index);         // 0x4f5de0, UNSURE signature
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); // 0x4f9a20, EAX object, stack (register_in_table, invoke_callback)
extern int32_t player_index_from_unit_index(uint32_t unit_index);                                   // 0x474db0, UNSURE signature
extern void equipment_pickup_play_sound(uint32_t object_index); // 0x4bbb50, EAX
extern void item_set_holder(uint32_t item_index, datum_index holder_index); // 0x4bcfc0, ECX item, EDX holder

uint8_t unit_try_select_equipment(uint32_t unit_index, uint32_t new_equipment_object_index, int16_t release_current)
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    if ((unit->equipment_object_index != k_datum_index_none) && (release_current == 1)) {
        object_delete(unit->equipment_object_index);
        unit->equipment_object_index = k_datum_index_none;
    }
    if (unit->equipment_object_index == k_datum_index_none) {
        object_unlink_cluster_or_notify_parent(new_equipment_object_index);
        object *new_obj = ((object_header *)object_data->data)[new_equipment_object_index & 0xffff].data;
        Object *new_def = (Object *)tag_instances[new_obj->definition_tag & 0xffff].data;
        if ((*(uint32_t *)&new_def->model.tag_id != 0xffffffff) && ((new_obj->flags & 1) == 0)) {
            object_for_each_light_attachment(new_equipment_object_index, 1, 0);
        }
        new_obj->flags |= 1;
        ((object_header *)object_data->data)[new_equipment_object_index & 0xffff].flags &= 0xfd;

        int32_t local_player = player_index_from_unit_index(unit_index);
        if (local_player != -1) {
            uint32_t local_player2 = (uint32_t)player_index_from_unit_index(unit_index);
            if (*(int16_t *)((uint8_t *)player_data->data + (local_player2 & 0xffff) * 0x200 + 2) != -1) {
                equipment_pickup_play_sound(new_equipment_object_index); // FIXED: 0x56d28f: EAX = EBX, the new equipment
            }
        }
        item_set_holder(new_equipment_object_index, unit_index); // 0x56d296: ECX = ebx, EDX = esi = unit_index
        unit->equipment_object_index = new_equipment_object_index;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x56d1a0):

undefined4 FUN_0056d1a0(uint param_1,uint param_2,short param_3)

{
  int iVar1;
  uint *puVar2;
  uint uVar3;
  int iVar4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if ((*(int *)(iVar1 + 0x318) != -1) && (param_3 == 1)) {
    object_delete();
    *(undefined4 *)(iVar1 + 0x318) = 0xffffffff;
  }
  if (*(int *)(iVar1 + 0x318) == -1) {
    object_unlink_cluster_or_notify_parent();
    iVar4 = (param_2 & 0xffff) * 0xc;
    puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar4);
    if ((*(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x34) != -1) &&
       ((puVar2[4] & 1) == 0)) {
      object_for_each_light_attachment(1,0);
    }
    iVar4 = *(int *)(DAT_008603b0 + 0x34) + iVar4;
    puVar2[4] = puVar2[4] | 1;
    *(byte *)(iVar4 + 2) = *(byte *)(iVar4 + 2) & 0xfd;
    iVar4 = FUN_00474db0(param_1);
    if (iVar4 != -1) {
      uVar3 = FUN_00474db0(param_1);
      if (*(short *)((uVar3 & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)) != -1) {
        FUN_004bbb50();
      }
    }
    FUN_004bcfc0();
    *(uint *)(iVar1 + 0x318) = param_2;
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
