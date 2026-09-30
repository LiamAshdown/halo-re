// unit_pickup_weapon  (Ghidra: FUN_0056d400)
// address 0x56d400, size 519 bytes, name confidence 0.4, rewrite confidence 0.9 (verified against objdump 0x56d400..0x56d606)
// functions.md: "Handles a unit picking up a nearby weapon object into a free inventory slot,
// updating attachment/physics state and optionally switching to it immediately."
// evidence: types/units.h unit_data.weapons[4] (0x2f8), .weapon_ready_ticks[4] (0x308),
//   .control_flags (0x208, bit 0x800 = _unit_control_flag_secondary_trigger),
//   .desired_weapon_index (0x2f4), .current_weapon_index (0x2f2), .animation_state (0x2a3,
//   0x1b = seat_exit); types/objects.h object.flags (0x10, bit 0x800), object.parent_object
//   (0x11c).
// blam-cc: EAX -> weapon_index, ECX -> unit_index, stack -> pickup_mode
//   for the unit (mask 3) and the weapon (mask 4) -- their ECX arguments are not visible here.
// UNSURE: unit_check_weapon_use_permission/game_engine_notify_weapon_ready_state_change/unit_set_local_player_weapon_index's real roles are not recovered.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_game.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern char k_empty_string[];      // 0x0065512c, UNSURE exact text FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)
extern int16_t network_game_mode; // 0x00719720 (a WORD; 0x719722 is the screenshot counter)

extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index);         // 0x4f5de0, UNSURE signature
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); // 0x4f9a20, EAX object
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label, uint8_t test_only); // 0x5651e0
extern char * unit_get_seat_or_state_name(uint32_t unit_index);                                    // 0x56c2f0, UNSURE signature
extern void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event); // 0x56c640
extern void unit_drop_inventory_weapons_except_current(uint32_t unit_index);        // 0x56d360
extern int16_t unit_find_empty_weapon_slot(uint32_t unit_index);                                  // 0x56d660, UNSURE signature
extern uint8_t unit_check_weapon_use_permission(uint32_t unit_index, uint32_t weapon_index); // 0x56da00, ESI unit, EDI weapon
extern int16_t unit_find_next_zone_permitted_weapon_slot(uint32_t unit_index, int32_t start_slot, int16_t direction); // 0x56dba0, EAX unit

extern void item_set_holder(uint32_t item_index, datum_index holder_index); // 0x4bcfc0, ECX item, EDX holder


uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index)
    // blam-cc: EAX -> weapon_index, ECX -> unit_index, stack -> pickup_mode
    // FIXED (objdump 0x56d400): ECX is the unit (object_try_and_get(ECX, mask 3) at 0x56d40f), EAX the weapon (mask 4);
    //   one stack argument, every caller cleans 4 bytes
{
    object *unit_obj = object_try_and_get(unit_index, _object_mask_unit);
    object *weapon_obj = object_try_and_get(weapon_index, _object_mask_weapon);
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    if (network_game_mode == 1) {
        char *seat_name = unit_get_seat_or_state_name(unit_index);
        char *weapon_label = k_empty_string;
        if (weapon_index != 0xffffffff) {
            object *label_src = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
            weapon_label = (char *)(tag_instances[label_src->definition_tag & 0xffff].data) + 0x30c;
        }
        if (unit_set_or_test_seat_and_weapon_label(unit_index, seat_name, weapon_label, 0) == 0) {
            object *check = object_try_and_get(unit_index, _object_mask_unit);
            if ((check != (object *)0) && (((unit_data *)((uint8_t *)check + k_unit_data_offset))->animation_state == 0x1b)) {
                unit_detach_from_seat(unit_index, 1, 1, 0); // 0x56d49f: push 0, 1, 1, esi
            }
        }
    }

    if (((weapon_obj->flags & 0x800) != 0) && (weapon_obj->parent_object == k_datum_index_none)) {
        if (unit_check_weapon_use_permission(unit_index, weapon_index) != 0 /* 0x56d4c7: ESI unit, EDI weapon */) {
            if (game_engine_notify_weapon_ready_state_change(unit_index, weapon_index) != 0 /* push edi (weapon); push esi (unit) at 0x56d4d4 */) {
                if (pickup_mode == 2) {
                    unit_drop_inventory_weapons_except_current(unit_index);
                }
                int16_t slot = unit_find_empty_weapon_slot(unit_index);
                if (slot != -1) {
                    object_unlink_cluster_or_notify_parent(weapon_index);
                    Object *weapon_def = (Object *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
                    if ((*(uint32_t *)&weapon_def->model.tag_id != 0xffffffff) &&   // tag + 0x34
                        ((weapon_obj->flags & 1) == 0)) {                          // puVar2[4], object + 0x10
                        object_for_each_light_attachment(weapon_index, 1, 0); // 0x56d54e: EAX = edi = weapon, push 0, push 1
                    }
                    weapon_obj->flags |= 1;
                    ((object_header *)object_data->data)[weapon_index & 0xffff].flags &= 0xfd;
                    item_set_holder(weapon_index, unit_index); // 0x56d56f: EDX = esi = the unit (ECX at entry), ECX = edi = weapon
                    unit->weapons[slot] = weapon_index;
                    unit->weapon_ready_ticks[slot] = 0;

                    if (pickup_mode != 0) {
                        if (pickup_mode == 1) {
                            if ((unit->control_flags & 0x800) == 0) {
                                unit_set_local_player_weapon_index(unit_index, slot); // 0x56d5ba: EAX = esi unit, push ebp = slot
                            }
                        } else if (pickup_mode != 2) {
                            return 1;
                        }
                        unit->desired_weapon_index = slot;
                        return 1;
                    }
                    unit->desired_weapon_index = unit_find_next_zone_permitted_weapon_slot(unit_index,
                        (int32_t)(uint16_t)unit->current_weapon_index, 0); // 0x56d5d8: EAX unit, the index zero-extended
                    return 1;
                }
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x56d400):

undefined4 FUN_0056d400(short param_1)

{
  byte *pbVar1;
  uint *puVar2;
  char cVar3;
  short sVar4;
  undefined2 uVar5;
  uint in_EAX;
  int iVar6;
  int iVar7;
  undefined4 uVar8;
  int iVar9;
  undefined1 *puVar10;

  iVar6 = object_try_and_get(3);
  iVar7 = object_try_and_get(4);
  if (DAT_00719720 == 1) {
    uVar8 = FUN_0056c2f0();
    puVar10 = &DAT_0065512c;
    if (in_EAX != 0xffffffff) {
      puVar10 = (undefined1 *)
                (*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc)
                          & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x30c);
    }
    cVar3 = unit_set_or_test_seat_and_weapon_label(uVar8,puVar10,0);
    if (cVar3 == '\0') {
      iVar9 = object_try_and_get(3);
      if ((iVar9 != 0) && (*(char *)(iVar9 + 0x2a3) == '\x1b')) {
        FUN_0056c640();
      }
    }
  }
  if (((*(uint *)(iVar7 + 0x10) & 0x800) != 0) && (*(int *)(iVar7 + 0x11c) == -1)) {
    cVar3 = FUN_0056da00();
    if (cVar3 != '\0') {
      cVar3 = FUN_00462000();
      if (cVar3 != '\0') {
        if (param_1 == 2) {
          FUN_0056d360();
        }
        uVar8 = FUN_0056d660();
        sVar4 = (short)uVar8;
        if (sVar4 != -1) {
          object_unlink_cluster_or_notify_parent();
          iVar7 = (in_EAX & 0xffff) * 0xc;
          puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
          if ((*(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x34) != -1) &&
             ((puVar2[4] & 1) == 0)) {
            object_for_each_light_attachment(1,0);
          }
          iVar9 = *(int *)(DAT_008603b0 + 0x34);
          puVar2[4] = puVar2[4] | 1;
          pbVar1 = (byte *)(iVar9 + iVar7 + 2);
          *pbVar1 = *pbVar1 & 0xfd;
          FUN_004bcfc0();
          *(uint *)(iVar6 + 0x2f8 + sVar4 * 4) = in_EAX;
          *(undefined4 *)(iVar6 + 0x308 + sVar4 * 4) = 0;
          if (param_1 != 0) {
            if (param_1 == 1) {
              if ((*(uint *)(iVar6 + 0x208) & 0x800) == 0) {
                FUN_00472100(uVar8);
              }
            }
            else if (param_1 != 2) {
              return 1;
            }
            *(short *)(iVar6 + 0x2f4) = sVar4;
            return 1;
          }
          uVar5 = FUN_0056dba0(*(undefined2 *)(iVar6 + 0x2f2),0);
          *(undefined2 *)(iVar6 + 0x2f4) = uVar5;
          return 1;
        }
      }
    }
  }
  return 0;
}
#endif
