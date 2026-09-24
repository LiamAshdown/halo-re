// first_person_weapon_process_action  (Ghidra: first_person_weapon_process_action, already named)
// address 0x4940f0, size 453 bytes
// name confidence: 0.5   rewrite confidence: 0.25
// evidence: types/interface.h first_person_weapon_interface names this address directly
// ("first_person_weapon_process_action @0x4940f0"); out/phase4/interface_functions.md "Central
// handler that applies a weapon HUD action code (charge, reload, swap, drop) to a local player's
// weapon interface and drives the resulting animation-state change."; types/items.h
// weapon_magazine_state (object+0x2b0, rounds_unloaded at +0x2b6, rounds_loaded at +0x2b8 read
// here as puVar3[0xac]/[0xae]-style dword-scaled indices); first_person_weapon_set_state.c's
// note on the weapon tag field at +0x4e2.
// register convention: local_player_index and action_code as the two recognized stack
// parameters (param_1, param_2).
// TYPES-GAP / UNSURE: this function writes to fp+0x1e90/0x1e92/0x1e94, which fall INSIDE
// types/interface.h's documented device_hud_element[0x44] array (0x1e10..0x1e98) rather than at
// a boundary -- an apparent field overlap this batch cannot resolve without editing that header;
// addressed here as raw offsets rather than through the named field. unit_invalidate_local_player_zoom_level and
// FUN_004927c0 (item_type_to_message_stage) are both called with zero visible arguments; the
// latter's own signature elsewhere takes an item_type_code, supplied here as action_code, which
// is a guess.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98
extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances; // 0x0087bc14

extern void unit_invalidate_local_player_zoom_level(void); // 0x4726f0, UNSURE args
extern int16_t item_type_to_message_stage(int16_t item_type_code); // 0x4927c0, this module
extern void first_person_weapon_set_state(int16_t local_player_index, uint8_t force_pose_snapshot,
                                           int16_t new_state); // 0x492e60
extern void first_person_weapon_interface_initialize(int16_t local_player_index); // 0x493c60

// Applies weapon HUD action `action_code` to local_player_index's first-person weapon interface:
// 0 nudges the charge float, 9/10 forward to unit_invalidate_local_player_zoom_level (presumably a reload/swap trigger), 12
// re-initializes the whole interface, 13 clears the weapon index. Then, unless the weapon index
// is already clear, validates the action against the weapon's magazine state (only for a
// "reload-family" animation state or an active magazine) and the weapon tag's +0x4e2 field
// (first_person_weapon_set_state.c's unresolved weapon-type-shaped enum, required == 1 for
// actions 9/10); on success, enters animation state 1. Action 12 additionally clears blend_end.
void first_person_weapon_process_action(int16_t local_player_index, int16_t action_code)
{
    first_person_weapon_interface *fp;
    uint8_t *fp_raw;

    if (local_player_index == -1) {
        return;
    }
    fp = &first_person_weapon_interfaces[local_player_index];
    fp_raw = (uint8_t *)fp;

    switch (action_code) {
        case 0:
            fp->charge = fp->charge + 0.05f;
            break;
        case 9:
        case 10:
            unit_invalidate_local_player_zoom_level();
            break;
        case 0xc:
            first_person_weapon_interface_initialize(local_player_index);
            break;
        case 0xd:
            fp->weapon_index = (datum_index)0xffffffff;
            break;
    }

    if (fp->weapon_index == (datum_index)0xffffffff) {
    retry_message_stage:
        if (item_type_to_message_stage(action_code) == -1) {
            goto skip_state_change;
        }
    } else {
        object *weapon_obj = *(object **)((char *)object_data->data + 8 +
                                           (uint16_t)fp->weapon_index * 0xc);
        char *weapon_tag_data;
        int32_t magazine_def;
        int16_t rounds_loaded_max;
        int16_t rounds_loaded;   // weapon_magazine_state.rounds_loaded, object+0x2b8
        int16_t rounds_unloaded; // weapon_magazine_state.rounds_unloaded, object+0x2b6
        int16_t magazine_state;  // weapon_magazine_state.state, object+0x2b0
        int32_t clamped;

        if (weapon_obj->definition_tag == (datum_index)0xffffffff) {
            goto retry_message_stage;
        }
        weapon_tag_data = (char *)tag_instances[(uint16_t)weapon_obj->definition_tag].data;
        if (*(int16_t *)(weapon_tag_data + 0x4e2) != 1) { // UNSURE, see first_person_weapon_set_state.c
            goto retry_message_stage;
        }
        if (action_code != 9 && action_code != 10) {
            goto retry_message_stage;
        }

        magazine_def = *(int32_t *)(weapon_tag_data + 0x4f4);
        magazine_state = *(int16_t *)((char *)weapon_obj + 0x2b0);
        rounds_loaded = *(int16_t *)((char *)weapon_obj + 0x2b8);
        rounds_unloaded = *(int16_t *)((char *)weapon_obj + 0x2b6);
        rounds_loaded_max = *(int16_t *)((char *)magazine_def + 10);

        clamped = (int32_t)rounds_loaded_max - (int32_t)rounds_loaded;
        if (rounds_unloaded < clamped) {
            clamped = rounds_unloaded;
        }

        if (fp->state == 0xf || fp->state == 0x16 || fp->state == 0x10 || fp->state == 0x11 ||
            fp->state == 0xd || fp->state == 0xe || magazine_state != 0) {
            *(int16_t *)(fp_raw + 0x1e94) = (clamped == 1) ? 1 : -1;
        } else {
            *(int16_t *)(fp_raw + 0x1e92) = (int16_t)clamped;
            *(uint8_t *)(fp_raw + 0x1e90) = (rounds_loaded == 0);
            *(int16_t *)(fp_raw + 0x1e94) = (clamped == 1) ? 2 : 0;
        }

        {
            int16_t marker = *(int16_t *)(fp_raw + 0x1e94);
            if (marker != -1 && marker != 0 && marker != 2) {
                goto retry_message_stage;
            }
        }
    }

    first_person_weapon_set_state(local_player_index, 1, 0); // UNSURE: new_state guessed, see
                                                               // first_person_weapon_interface_
                                                               // initialize.c's same guess
skip_state_change:
    if (action_code == 0xc) {
        fp->blend_end = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4940f0):

void first_person_weapon_process_action(undefined4 param_1,short param_2)

{
  short sVar1;
  short sVar2;
  uint *puVar3;
  short sVar4;
  int iVar5;
  int iVar6;

  if ((short)param_1 == -1) {
    return;
  }
  iVar6 = (short)param_1 * 0x1ea0 + DAT_006b2d98;
  switch((int)param_2) {
  case 0:
    *(float *)(iVar6 + 0x2c) = *(float *)(iVar6 + 0x2c) + 0.05;
    break;
  case 9:
  case 10:
    FUN_004726f0();
    break;
  case 0xc:
    FUN_00493c60(param_1);
    break;
  case 0xd:
    *(undefined4 *)(iVar6 + 8) = 0xffffffff;
  }
  if (*(uint *)(iVar6 + 8) == 0xffffffff) {
LAB_00494284:
    sVar4 = FUN_004927c0();
    if (sVar4 == -1) goto LAB_004942a0;
  }
  else {
    puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar6 + 8) & 0xffff) * 0xc);
    if (((*puVar3 == 0xffffffff) ||
        (iVar5 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
        *(short *)(iVar5 + 0x4e2) != 1)) || ((param_2 != 9 && (param_2 != 10)))) goto LAB_00494284;
    iVar5 = *(int *)(iVar5 + 0x4f4);
    sVar4 = *(short *)(iVar6 + 0xc);
    sVar1 = (short)puVar3[0xae];
    sVar2 = *(short *)((int)puVar3 + 0x2b6);
    if ((((((sVar4 == 0xf) || (sVar4 == 0x16)) || (sVar4 == 0x10)) ||
         ((sVar4 == 0x11 || (sVar4 == 0xd)))) || (sVar4 == 0xe)) || ((short)puVar3[0xac] != 0)) {
      iVar5 = (int)*(short *)(iVar5 + 10) - (int)sVar1;
      if (sVar2 < iVar5) {
        iVar5 = (int)sVar2;
      }
      if (iVar5 == 1) {
        *(undefined2 *)(iVar6 + 0x1e94) = 1;
      }
      else {
        *(undefined2 *)(iVar6 + 0x1e94) = 0xffff;
      }
    }
    else {
      iVar5 = (int)*(short *)(iVar5 + 10) - (int)sVar1;
      if (sVar2 < iVar5) {
        iVar5 = (int)sVar2;
      }
      *(short *)(iVar6 + 0x1e92) = (short)iVar5;
      *(bool *)(iVar6 + 0x1e90) = sVar1 == 0;
      *(ushort *)(iVar6 + 0x1e94) = ((short)iVar5 != 1) - 1 & 2;
    }
    sVar4 = *(short *)(iVar6 + 0x1e94);
    if (((sVar4 != -1) && (sVar4 != 0)) && (sVar4 != 2)) goto LAB_00494284;
  }
  first_person_weapon_set_state(param_1,1);
LAB_004942a0:
  if (param_2 == 0xc) {
    *(undefined2 *)(iVar6 + 0x8a) = 0;
  }
  return;
}
#endif
