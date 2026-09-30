// player_check_vehicle_boarding_interaction  (Ghidra: FUN_004788a0)
// address 0x4788a0, size 926 bytes
// name confidence: 0.2 (it is really the player's item-touch handler; the name is kept for the symbol table)
// rewrite confidence: 0.85
// REWRITTEN from objdump 0x4788a0..0x478c3d (the draft dropped the register arguments of every pickup, HUD and
//   interaction call). Stack: (player, item). Items still attached to something, or last dropped by this
//   unit (+0x200), are ignored. Ammo: the first carried weapon that takes ammunition from the item
//   (0x4c2610) posts the count on the HUD (0x4ae350, kind 1). Equipment (mask 8): type 6 (+0x308) gives a
//   grenade (0x56d080) and posts it (kind 0xff); other types apply the powerup (0x479930) unless the unit
//   holds one (+0x318). Weapons (mask 4) the unit may use (0x56da00): not while dual-flagged (+0x208 bits
//   0x1800) for a weapon flagged 8; when the player is free to take it (0x478820) the weapon is picked up
//   (0x56d400) and shown (host: 0x4ae350 kind 0, else 0x4ae400), the zoom reset and a host notifies the
//   engine (0x478ff0: 1, 7, -1, -1); otherwise, unless an unflagged weapon would replace a flagged current one
//   with two or more carried, a better weapon (0x56dae0) becomes the pending action (0x478e00): 7 when the
//   unit carries one weapon of a different tag, else 6 (swap).
// blam-cc: stack -> (player_index, candidate_object)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "items.h"
#include "fn_game.h"
#include "fn_items.h"
#include "fn_interface.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t network_game_mode;  // 0x00719720, 2 = host


extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern uint8_t unit_try_give_grenade(uint32_t tag_source_index, uint32_t unit_index); // 0x56d080, stack, EBX
extern void player_apply_pickup_effect(uint32_t player_index, uint32_t pickup_object); // 0x479930

extern uint8_t unit_check_weapon_use_permission(uint32_t unit_index, uint32_t weapon_index); // 0x56da00, ESI, EDI
extern int16_t unit_count_deployed_weapons(uint32_t unit_index); // 0x56d990, EAX

extern uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index); // 0x56d400, stack, EAX, ECX
extern uint8_t unit_weapon_is_best_of_type(uint32_t reference_weapon_index, uint32_t unit_index); // 0x56dae0, EAX, ECX
extern void unit_invalidate_local_player_zoom_level(datum_index unit); // 0x4726f0, EAX


#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

void player_check_vehicle_boarding_interaction(uint32_t player_index, uint32_t candidate_object)
{
    uint8_t *record = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    datum_index unit_index = ((player *)record)->unit;
    uint8_t *unit = OBJECT_DATA(unit_index);
    uint8_t *item = OBJECT_DATA(candidate_object);
    int16_t local_player_index = ((player *)record)->local_player_index;
    int8_t machine = (int8_t)record[0x64];
    uint8_t *equipment;
    uint8_t *weapon;
    uint8_t *weapon_tag;
    uint8_t dual_flagged;
    uint8_t keep_current;
    datum_index current_weapon;
    int32_t weapon_count;
    int16_t i;

    if (((struct item_object *)item)->base.parent_object != k_datum_index_none || ((struct item_object *)item)->item.ignore_object_index == unit_index) {
        return;
    }
    for (i = 0; i < 4; i++) {
        datum_index carried = *(datum_index *)(unit + 0x2f8 + i * 4);
        int16_t transferred;

        if (carried != k_datum_index_none &&
            (uint8_t)weapon_transfer_ammunition(carried, candidate_object, local_player_index, &transferred)) {
            if (transferred > 0) {
                hud_post_item_message(transferred, (int32_t)*(datum_index *)OBJECT_DATA(carried), 1,
                    local_player_index, machine);
            }
            break;
        }
    }

    equipment = (uint8_t *)object_try_and_get(candidate_object, 8);
    if (equipment != 0) {
        uint8_t *equipment_tag = TAG_DATA(*(datum_index *)equipment);
        int16_t type = *(int16_t *)(equipment_tag + 0x308);

        if (type == 6) {
            if (unit_try_give_grenade(candidate_object, unit_index)) {
                hud_post_item_message(1, (int32_t)*(datum_index *)equipment, 0xff, local_player_index, machine);
            }
        } else if (type != 0) {
            if (*(datum_index *)(OBJECT_DATA(unit_index) + 0x318) == k_datum_index_none) {
                player_apply_pickup_effect(player_index, candidate_object);
            } else if (type != *(int16_t *)(equipment_tag + 0x308)) {
                // 0x478a1a compares the type with the field it was read from: never taken
                player_set_pending_interaction_action(5, -1, player_index, candidate_object);
            }
        }
    }

    weapon = (uint8_t *)object_try_and_get(candidate_object, 4);
    if (weapon == 0 || !unit_check_weapon_use_permission(unit_index, candidate_object)) {
        return;
    }
    weapon_tag = TAG_DATA(*(datum_index *)weapon);
    dual_flagged = (uint8_t)((((unit_object *)unit)->unit.control_flags & 0x1800) != 0);
    {
        uint8_t *holder = OBJECT_DATA(unit_index);
        int16_t slot = *(int16_t *)(holder + 0x2f2);

        current_weapon = (slot != -1) ? *(datum_index *)(holder + 0x2f8 + slot * 4) : k_datum_index_none;
    }
    weapon_count = unit_count_deployed_weapons(unit_index);
    keep_current = 0;
    if (weapon_count >= 2 && current_weapon != k_datum_index_none && (weapon_tag[0x308] & 0x10) == 0 &&
        (TAG_DATA(*(datum_index *)OBJECT_DATA(current_weapon))[0x308] & 0x10) != 0) {
        keep_current = 1;
    }
    if (dual_flagged && (weapon_tag[0x308] & 8)) {
        return;
    }
    if (player_is_busy_with_interaction(candidate_object, unit_index)) {
        datum_index tag;

        if (!unit_pickup_weapon(1, candidate_object, unit_index)) {
            return;
        }
        tag = *(datum_index *)OBJECT_DATA(candidate_object);
        if (network_game_mode == 2) {
            hud_post_item_message(0, (int32_t)tag, 0, local_player_index, machine);
        } else {
            hud_add_item_message(local_player_index, (int32_t)tag, 0, 0);
        }
        unit_invalidate_local_player_zoom_level(unit_index);
        if (network_game_mode == 2) {
            game_engine_notify_player_interaction(player_index, candidate_object, 1, 7, -1, -1);
        }
        return;
    }
    if (keep_current || !unit_weapon_is_best_of_type(candidate_object, unit_index)) {
        return;
    }
    {
        uint8_t *current = (uint8_t *)object_try_and_get(current_weapon, 4);

        if (weapon_count == 1 && current != 0 && *(datum_index *)current != *(datum_index *)weapon) {
            player_set_pending_interaction_action(7, -1, player_index, candidate_object);
        } else {
            player_set_pending_interaction_action(6, -1, player_index, candidate_object);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4788a0), from tools/pack.py 0x4788a0:

void FUN_004788a0(uint param_1,uint param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  bool bVar4;
  char cVar5;
  short sVar6;
  uint uVar7;
  uint *puVar8;
  uint uVar9;
  int iVar10;
  undefined4 uVar11;
  uint *local_8;
  int local_4;

  iVar10 = (param_1 & 0xffff) * 0x200;
  uVar7 = *(uint *)(iVar10 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
  iVar10 = iVar10 + *(int *)(DAT_0087a480 + 0x34);
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
  local_4 = (param_2 & 0xffff) * 0xc;
  iVar2 = *(int *)(local_4 + 8 + *(int *)(DAT_008603b0 + 0x34));
  if ((*(int *)(iVar2 + 0x11c) == -1) && (*(uint *)(iVar2 + 0x200) != uVar7)) {
    sVar6 = 0;
    do {
      uVar7 = *(uint *)(iVar1 + 0x2f8 + sVar6 * 4);
      if ((uVar7 != 0xffffffff) &&
         (uVar7 = item_transfer_ammunition(uVar7,param_2,*(short *)(iVar10 + 2),(short *)&local_8),
         (char)uVar7 != '\0')) {
        if (0 < (short)local_8) {
          FUN_004ae350(*(undefined2 *)(iVar10 + 2),*(undefined1 *)(iVar10 + 100));
        }
        break;
      }
      sVar6 = sVar6 + 1;
    } while (sVar6 < 4);
    puVar8 = (uint *)object_try_and_get(8);
    if (puVar8 != (uint *)0x0) {
      iVar2 = *(int *)((*puVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      sVar6 = *(short *)(iVar2 + 0x308);
      if (sVar6 == 6) {
        cVar5 = FUN_0056d080(param_2);
        if (cVar5 != '\0') {
          FUN_004ae350(*(undefined2 *)(iVar10 + 2),*(undefined1 *)(iVar10 + 100));
        }
      }
      else if (sVar6 != 0) {
        if (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                             (*(uint *)(iVar10 + 0x34) & 0xffff) * 0xc) + 0x318) == -1) {
          FUN_00479930(param_1,param_2);
        }
        else if (sVar6 != *(short *)(iVar2 + 0x308)) {
          player_set_pending_interaction_action(5,0xffffffff);
        }
      }
    }
    local_8 = (uint *)object_try_and_get(4);
    if ((local_8 != (uint *)0x0) && (cVar5 = FUN_0056da00(), iVar2 = DAT_008603b0, cVar5 != '\0')) {
      iVar3 = *(int *)((*local_8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      uVar7 = *(uint *)(iVar1 + 0x208);
      iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar10 + 0x34) & 0xffff) * 0xc
                      );
      sVar6 = *(short *)(iVar1 + 0x2f2);
      uVar9 = 0xffffffff;
      if (sVar6 != -1) {
        uVar9 = *(uint *)(iVar1 + 0x2f8 + sVar6 * 4);
      }
      sVar6 = FUN_0056d990();
      bVar4 = false;
      if ((((1 < sVar6) && (uVar9 != 0xffffffff)) && ((*(byte *)(iVar3 + 0x308) & 0x10) == 0)) &&
         ((*(byte *)(*(int *)((**(uint **)(*(int *)(iVar2 + 0x34) + 8 + (uVar9 & 0xffff) * 0xc) &
                              0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) & 0x10) != 0)) {
        bVar4 = true;
      }
      if (((uVar7 & 0x1800) == 0) || ((*(byte *)(iVar3 + 0x308) & 8) == 0)) {
        cVar5 = FUN_00478820();
        if (cVar5 == '\0') {
          if ((!bVar4) && (cVar5 = FUN_0056dae0(), cVar5 != '\0')) {
            puVar8 = (uint *)object_try_and_get(4);
            if ((sVar6 == 1) && ((puVar8 != (uint *)0x0 && (*puVar8 != *local_8)))) {
              uVar11 = 7;
            }
            else {
              uVar11 = 6;
            }
            player_set_pending_interaction_action(uVar11,0xffffffff);
          }
        }
        else {
          cVar5 = FUN_0056d400(1);
          if (cVar5 != '\0') {
            if (DAT_00719720 == 2) {
              FUN_004ae350(*(undefined2 *)(iVar10 + 2),*(undefined1 *)(iVar10 + 100));
            }
            else {
              FUN_004ae400(0);
            }
            FUN_004726f0();
            if (DAT_00719720 == 2) {
              FUN_00478ff0(1,7,0xffffffff,0xffffffff);
              return;
            }
          }
        }
      }
    }
  }
}
#endif
