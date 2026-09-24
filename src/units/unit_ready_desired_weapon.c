// unit_ready_desired_weapon  (Ghidra: unit_ready_desired_weapon, already named)
// address 0x56d6e0, size 679 bytes, name confidence 0.6, rewrite confidence 0.2
// functions.md: "Switches the unit from its current weapon to its desired inventory slot (or to
// being unarmed if none is available), updating attachment and animation state accordingly."
// evidence: types/units.h unit_data.desired_weapon_index (0x2f4), .current_weapon_index (0x2f2),
//   .weapons[4] (0x2f8), .animation_weapon_index (0x2a1), .animation_definition_index (0x2a0),
//   .weapon_ready_ticks[4] (0x308, "puVar3[seat+0xc2]" lands here); cea-pdb confirms the name via
//   the "unarmed" string.
// blam-cc: param_1 -> unit_index.
// UNSURE: unit_get_seat_or_state_name is called here with two visible arguments (a value and a literal 1, or a
// literal "unarmed" string and 1), which contradicts that same address's own decompilation
// elsewhere in this module (a single-argument, EAX-only function returning the unit's current
// seat/state name -- see unit_get_seat_or_state_name.c). This rewrite reproduces the call
// exactly as shown, with a 2-argument extern local to this file, rather than reconciling the two
// readings; whichever is correct, the visible effect is limited to which string
// unit_set_or_test_seat_and_weapon_label receives as its seat label, not the weapon-switch
// bookkeeping this function performs directly.
// UNSURE: weapon_put_away, weapon_get_label and weapon_ready's real roles are not recovered.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern hs_game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/hs.h)
extern char *s_unarmed;             // "unarmed"

extern void item_set_holder(datum_index object_index);                                // 0x4bcfc0, UNSURE signature
extern int32_t weapon_get_label(void);                                                 // 0x4c24d0, UNSURE signature
extern void weapon_ready(void);                                                    // 0x4c2840, UNSURE signature
extern uint8_t weapon_put_away(void);                                                 // 0x4c28f0, UNSURE signature
extern void object_mark_pending_delete(uint32_t object_index);                     // 0x4f50f0, UNSURE signature
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location);   // 0x4f5c30, UNSURE signature
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index);         // 0x4f5de0, UNSURE signature
extern void object_reorient_relative_to_marker(uint32_t unit_index, uint8_t *marker); // 0x4f6180, UNSURE signature
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);                                   // 0x4f6610, UNSURE signature
extern void object_for_each_light_attachment(uint32_t object_index, uint32_t flag); // 0x4f9a20, UNSURE signature  // real signature (object_for_each_light_attachment.c): void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); Ghidra recovered 2 of 3 args at this call site
extern uint8_t unit_set_or_test_seat_and_weapon_label(char *seat_label);           // 0x5651e0, UNSURE 1-arg call site  // real signature (unit_set_or_test_seat_and_weapon_label.c): uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label, uint8_t test_only); Ghidra recovered 1 of 4 args at this call site
extern void unit_validate_and_clear_weapon_switch(uint32_t unit_index);                                     // 0x5659c0, UNSURE signature
extern char *unit_get_seat_or_state_name(int32_t a, int32_t b);                                   // 0x56c2f0, UNSURE 2-arg call site, see header  // real signature (unit_get_seat_or_state_name.c): char * unit_get_seat_or_state_name(uint32_t unit_index); Ghidra recovered 2 of 1 args at this call site

void unit_ready_desired_weapon(uint32_t unit_index)
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;

    datum_index desired_weapon = k_datum_index_none;
    if (unit->desired_weapon_index != -1) {
        desired_weapon = unit->weapons[unit->desired_weapon_index];
    }

    if (unit->current_weapon_index != -1) {
        datum_index current_weapon = unit->weapons[unit->current_weapon_index];
        if (current_weapon != k_datum_index_none) {
            if (weapon_put_away() != 0) {
                object_snap_to_parent_marker_and_detach(current_weapon);
                object_unlink_cluster_or_notify_parent(current_weapon);
                object_mark_pending_delete(current_weapon);
                object *weapon_obj = ((object_header *)object_data->data)[current_weapon & 0xffff].data;
                Object *weapon_def = (Object *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
                if ((*(uint32_t *)&weapon_def->model.tag_id != 0xffffffff) &&   // tag + 0x34
                    ((weapon_obj->flags & 1) == 0)) {                          // puVar6[4], object + 0x10
                    object_for_each_light_attachment(current_weapon, 0);
                }
                weapon_obj->flags |= 1;                                        // puVar6[4] |= 1
                ((object_header *)object_data->data)[current_weapon & 0xffff].flags &= 0xfd;
                item_set_holder(current_weapon);
                unit->current_weapon_index = -1;
            }
        }
    }

    if (unit->current_weapon_index == -1) {
        if (desired_weapon != k_datum_index_none) {
            int32_t a = weapon_get_label();
            char *label = unit_get_seat_or_state_name(a, 1);
            unit_set_or_test_seat_and_weapon_label(label);

            int8_t weapon_index_in_seat = unit->animation_weapon_index;
            uint8_t *graph = (uint8_t *)tag_instances[unit_tag->base.animation_graph.tag_id.index & 0xffff].data;
            uint8_t *units_block = *(uint8_t **)(graph + 0x10);
            uint8_t *weapons_array = *(uint8_t **)(units_block + 0x5c + unit->animation_definition_index * 100);

            object_set_cluster_and_parent(desired_weapon, 0);
            object *weapon_obj = ((object_header *)object_data->data)[desired_weapon & 0xffff].data;
            Object *weapon_def = (Object *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
            if (*(uint32_t *)&weapon_def->model.tag_id != 0xffffffff) {        // tag + 0x34
                if ((weapon_obj->flags & 1) != 0) {                            // puVar6[4]
                    object_for_each_light_attachment(desired_weapon, 1);
                }
                if (*(uint32_t *)&weapon_def->model.tag_id != 0xffffffff) {
                    weapon_obj->flags &= ~1u;                                  // puVar6[4] &= 0xfffffffe
                    ((object_header *)object_data->data)[desired_weapon & 0xffff].flags |= 0x02;
                }
            }
            uint8_t *ready_marker = weapons_array + weapon_index_in_seat * 0xbc + 0x40;
            object_reorient_relative_to_marker(unit_index, ready_marker);

            int16_t new_current = unit->desired_weapon_index;
            unit->current_weapon_index = new_current;
            if (new_current != -1) {
                unit->weapon_ready_ticks[new_current] = game_time->current_tick;
            }
            weapon_ready();
            unit_validate_and_clear_weapon_switch(unit_index);
            return;
        }
        char *label = unit_get_seat_or_state_name((int32_t)(intptr_t)s_unarmed, 1);
        unit_set_or_test_seat_and_weapon_label(label);
        unit->current_weapon_index = -1;
    }
    unit_validate_and_clear_weapon_switch(unit_index);
    return;
}

#if 0
Original Ghidra decompilation (0x56d6e0):

void unit_ready_desired_weapon(uint param_1)

{
  byte *pbVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  uint *puVar6;
  char cVar7;
  int iVar8;
  undefined4 uVar9;
  uint uVar10;
  int iVar11;
  undefined4 uVar12;

  iVar8 = (param_1 & 0xffff) * 0xc;
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
  iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar10 = 0xffffffff;
  if ((short)puVar3[0xbd] != -1) {
    uVar10 = puVar3[(short)puVar3[0xbd] + 0xbe];
  }
  iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
  sVar2 = *(short *)(iVar8 + 0x2f2);
  if ((sVar2 != -1) && (uVar5 = *(uint *)(iVar8 + 0x2f8 + sVar2 * 4), uVar5 != 0xffffffff)) {
    cVar7 = FUN_004c28f0();
    if (cVar7 != '\0') {
      FUN_004f6610(uVar5);
      object_unlink_cluster_or_notify_parent();
      object_mark_pending_delete();
      iVar8 = (uVar5 & 0xffff) * 0xc;
      puVar6 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
      if ((*(int *)(*(int *)((*puVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x34) != -1) &&
         ((puVar6[4] & 1) == 0)) {
        object_for_each_light_attachment(1,0);
      }
      iVar11 = *(int *)(DAT_008603b0 + 0x34);
      puVar6[4] = puVar6[4] | 1;
      pbVar1 = (byte *)(iVar11 + iVar8 + 2);
      *pbVar1 = *pbVar1 & 0xfd;
      FUN_004bcfc0();
      *(undefined2 *)((int)puVar3 + 0x2f2) = 0xffff;
    }
  }
  if (*(short *)((int)puVar3 + 0x2f2) == -1) {
    uVar12 = 1;
    if (uVar10 != 0xffffffff) {
      uVar9 = FUN_004c24d0();
      uVar12 = FUN_0056c2f0(uVar9,uVar12);
      unit_set_or_test_seat_and_weapon_label(uVar12);
      cVar7 = *(char *)((int)puVar3 + 0x2a1);
      iVar4 = *(int *)(*(int *)(*(int *)((*(uint *)(iVar4 + 0x44) & 0xffff) * 0x20 + 0x14 +
                                        DAT_0087bc14) + 0x10) + 0x5c + (char)puVar3[0xa8] * 100);
      object_set_cluster_and_parent(uVar10,0);
      iVar11 = (uVar10 & 0xffff) * 0xc;
      puVar6 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar11);
      iVar8 = *(int *)((*puVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (*(int *)(iVar8 + 0x34) != -1) {
        if ((puVar6[4] & 1) != 0) {
          object_for_each_light_attachment(0,1);
        }
        if (*(int *)(iVar8 + 0x34) != -1) {
          iVar8 = *(int *)(DAT_008603b0 + 0x34);
          puVar6[4] = puVar6[4] & 0xfffffffe;
          pbVar1 = (byte *)(iVar8 + iVar11 + 2);
          *pbVar1 = *pbVar1 | 2;
        }
      }
      object_reorient_relative_to_marker(param_1,cVar7 * 0xbc + iVar4 + 0x40);
      sVar2 = (short)puVar3[0xbd];
      *(short *)((int)puVar3 + 0x2f2) = sVar2;
      if (sVar2 != -1) {
        puVar3[sVar2 + 0xc2] = *(uint *)(DAT_006f1d6c + 0xc);
      }
      FUN_004c2840();
      FUN_005659c0(param_1);
      return;
    }
    uVar12 = FUN_0056c2f0("unarmed",1);
    unit_set_or_test_seat_and_weapon_label(uVar12);
    *(undefined2 *)((int)puVar3 + 0x2f2) = 0xffff;
  }
  FUN_005659c0(param_1);
  return;
}
#endif
