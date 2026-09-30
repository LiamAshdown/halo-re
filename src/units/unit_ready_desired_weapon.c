// unit_ready_desired_weapon  (Ghidra: unit_ready_desired_weapon, already named)
// address 0x56d6e0, size 679 bytes, name confidence 0.6, rewrite confidence 0.85
// REWRITTEN from objdump 0x56d6e0..0x56d986 (the draft dropped the second argument, called
//   unit_set_or_test_seat_and_weapon_label with one argument and most callees without theirs).
//   Stack: unit, force (every caller passes 1; it goes to weapon_put_away in AL).
//   The desired weapon is weapons[+0x2f4]. A current weapon (+0x2f2) that weapon_put_away(ESI weapon, AL force)
//   accepts is detached (0x4f6610 stack, 0x4f5de0 EAX, 0x4f50f0 pending delete EAX), its lights unregistered
//   (0x4f9a20 EAX, stack 1, 0) when it has a model (tag +0x34) and was visible (object +0x10 bit 0 clear), hidden
//   (+0x10 |= 1, header flag 2 cleared), handed to the unit as holder (ECX weapon, EDX unit) and +0x2f2 = -1.
//   With no current weapon left: a desired weapon sets the seat/weapon animation labels
//   (unit_set_or_test_seat_and_weapon_label(unit, seat name 0x56c2f0, weapon label 0x4c24d0, 1)), is placed with no
//   location (0x4f5c30 stack weapon, 0), shown again (lights registered EAX, stack 0, 1; +0x10 bit 0 cleared and
//   header flag 2 set when it has a model), attached to the unit's hand (0x4f6180 stack unit, weapon anim +0x40;
//   ESI weapon, EDI weapon anim +0x20), becomes current with its ready tick (+0x308[i] = game time) and is readied
//   (0x4c2840 EAX); otherwise the labels go to "unarmed" and +0x2f2 = -1. Both end in 0x5659c0 (stack unit).
// evidence: types/units.h unit_data.desired_weapon_index (0x2f4), .current_weapon_index (0x2f2),
//   .weapons[4] (0x2f8), .animation_weapon_index (0x2a1), .animation_definition_index (0x2a0),
//   .weapon_ready_ticks[4] (0x308).
// blam-cc: stack -> unit_index, force

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_units.h"
#include "fn_objects.h"
#include <stdint.h>

extern data_array *object_data;       // 0x008603b0
extern tag_instance *tag_instances;   // 0x0087bc14
extern game_time_globals *game_time;  // 0x006f1d6c

extern void item_set_holder(uint32_t item_index, datum_index holder_index); // 0x4bcfc0, ECX item, EDX holder
extern char *weapon_get_label(datum_index item_index); // 0x4c24d0, ECX
extern void weapon_ready(datum_index item_index); // 0x4c2840, EAX
extern int32_t weapon_put_away(datum_index item_index, int8_t force); // 0x4c28f0, ESI, AL
extern void object_mark_pending_delete(uint32_t object_index); // 0x4f50f0, EAX

extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0, EAX
extern void object_reorient_relative_to_marker(uint32_t parent_index, char *parent_marker_name,
    uint32_t object_index, char *object_marker_name); // 0x4f6180, stack, stack, ESI, EDI
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index); // 0x4f6610, stack
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
    int32_t invoke_callback); // 0x4f9a20, EAX, stack, stack
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label,
    uint8_t apply); // 0x5651e0

extern char *unit_get_seat_or_state_name(uint32_t unit_index); // 0x56c2f0, EAX

#define OBJECT_HEADER(h) (((object_header *)object_data->data)[(h) & 0xffff])
#define OBJECT_TAG(o) ((uint8_t *)tag_instances[*(datum_index *)(o) & 0xffff].data)

void unit_ready_desired_weapon(uint32_t unit_index, uint8_t force)
{
    uint8_t *unit = (uint8_t *)OBJECT_HEADER(unit_index).data;
    uint8_t *unit_tag = OBJECT_TAG(unit);
    datum_index desired_weapon = k_datum_index_none;
    int16_t current = ((unit_object *)unit)->unit.current_weapon_index;

    if (((unit_object *)unit)->unit.desired_weapon_index != -1) {
        desired_weapon = *(datum_index *)(unit + 0x2f8 + ((unit_object *)unit)->unit.desired_weapon_index * 4);
    }
    if (current != -1) {
        datum_index weapon = *(datum_index *)(unit + 0x2f8 + current * 4);

        if (weapon != k_datum_index_none && weapon_put_away(weapon, (int8_t)force) != 0) {
            uint8_t *weapon_obj;

            object_snap_to_parent_marker_and_detach(weapon);
            object_unlink_cluster_or_notify_parent(weapon);
            object_mark_pending_delete(weapon);
            weapon_obj = (uint8_t *)OBJECT_HEADER(weapon).data;
            if (*(int32_t *)(OBJECT_TAG(weapon_obj) + 0x34) != -1 && (weapon_obj[0x10] & 1) == 0) {
                object_for_each_light_attachment(weapon, 1, 0);
            }
            *(uint32_t *)(weapon_obj + 0x10) |= 1;
            OBJECT_HEADER(weapon).flags &= 0xfd;
            item_set_holder(weapon, unit_index);
            ((unit_object *)unit)->unit.current_weapon_index = -1;
        }
    }
    if (((unit_object *)unit)->unit.current_weapon_index != -1) {
        unit_validate_and_clear_weapon_switch(unit_index);
        return;
    }
    if (desired_weapon == k_datum_index_none) {
        unit_set_or_test_seat_and_weapon_label(unit_index, unit_get_seat_or_state_name(unit_index), "unarmed", 1);
        ((unit_object *)unit)->unit.current_weapon_index = -1;
        unit_validate_and_clear_weapon_switch(unit_index);
        return;
    }
    {
        char *weapon_label = weapon_get_label(desired_weapon);
        uint8_t *graph;
        uint8_t *weapon_anim;
        uint8_t *weapon_obj;
        uint8_t *weapon_tag;
        int16_t desired;

        unit_set_or_test_seat_and_weapon_label(unit_index, unit_get_seat_or_state_name(unit_index), weapon_label, 1);
        graph = (uint8_t *)tag_instances[*(datum_index *)(unit_tag + 0x44) & 0xffff].data;
        weapon_anim = *(uint8_t **)(*(uint8_t **)&((ModelAnimations *)graph)->units.pointer + (int8_t)unit[0x2a0] * 0x64 + 0x5c) +
            (int8_t)unit[0x2a1] * 0xbc;
        object_set_cluster_and_parent(desired_weapon, 0);
        weapon_obj = (uint8_t *)OBJECT_HEADER(desired_weapon).data;
        weapon_tag = OBJECT_TAG(weapon_obj);
        if (*(int32_t *)(weapon_tag + 0x34) != -1) {
            if ((weapon_obj[0x10] & 1) != 0) {
                object_for_each_light_attachment(desired_weapon, 0, 1);
            }
            if (*(int32_t *)(weapon_tag + 0x34) != -1) {
                *(uint32_t *)(weapon_obj + 0x10) &= ~1u;
                OBJECT_HEADER(desired_weapon).flags |= 2;
            }
        }
        object_reorient_relative_to_marker(unit_index, (char *)(weapon_anim + 0x40), desired_weapon,
            (char *)(weapon_anim + 0x20));
        desired = ((unit_object *)unit)->unit.desired_weapon_index;
        ((unit_object *)unit)->unit.current_weapon_index = desired;
        if (desired != -1) {
            *(int32_t *)(unit + 0x308 + desired * 4) = game_time->game_time;
        }
        weapon_ready(desired_weapon);
        unit_validate_and_clear_weapon_switch(unit_index);
    }
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
