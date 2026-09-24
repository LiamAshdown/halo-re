// hud_update_interaction_prompt  (Ghidra: FUN_004a9b80, renamed; earlier
// hud_weapon_message_state_update)
// address 0x4a9b80, size 1632 bytes
// name confidence: 0.5 (chosen)   rewrite confidence: 0.6
// evidence: rewritten in the phase-4 review from objdump -d 0x4a9b80..0x4aa261 plus the two jump
// tables at 0x4aa264 (player_globals::mode 1..4) and 0x4aa274 (player::interaction_type 1..11).
// Every path picks one hud action message through FUN_004adfc0 (EAX message index, stack local
// player; it points hud_player_messaging_state +0x454 at hud_globals message element index*0x40)
// and fills its arguments through hud_message_set_string_argument @0x4ae0b0 (EAX local player,
// ESI argument slot, stack int16 string index and a flag byte) or
// hud_message_set_numeric_argument @0x4ae050 (EAX local player, ESI slot, stack dword; here a
// pointer to WeaponHUDInterface::messaging_information_sequence_index at +0x13c). The string
// indices are Object::hud_text_message_index (+0x13c of an object tag), read directly or through
// object_get_hud_text_message_index @0x4a9b40.
// The earlier rewrite had an opaque table for the no-unit modes, one merged case for
// interaction types 6 and 7 (they differ in the message index, 4 and 0), no message index at
// all, argument slots 0 where the binary uses 1, every register argument dropped, and the
// weapon cycling loop running on garbage slot values.
// Interaction types and messages: 1, 2 message 0 with the target name; 3 message 7 with the
// vehicle the unit rides; 5 message 1 with the held object then the target; 6 and 7 messages 4
// and 0 with the weapon HUD icon of the target (or its name); 8, 9 message 6 with the seat text;
// 10 message 3 with the vehicle +0x218 string (flag 1) or message 2; 11 message 8.
// Anything else: a game engine hint (game_engine_pick_hud_hint @0x463150) wins; otherwise when
// the current weapon is empty (weapon_hud_ammo_state_is_empty @0x4a9750) the next non-empty
// weapon slot is looked for and message 5 names it.
// UNSURE: the weapon cycling loop only repeats while unit_count_deployed_weapons minus one is 0,
// exactly as the binary tests it (dec, test ax, je back); kept as is.
// register convention: player index (datum) in EDX.
//   // blam-cc: player_index -> EDX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include "interface.h"
#include <wchar.h>

extern tag_instance *tag_instances;          // 0x0087bc14
extern data_array *object_data; // 0x008603b0
extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480
extern int16_t current_local_player_index; // 0x007c3108
extern hud_messaging_globals *hud_messaging; // 0x006b3a40

extern void hud_set_player_message(int16_t message_index, int16_t local_player_index); // 0x4adfc0, blam-cc: EAX message_index
extern void hud_set_message_string_argument(int16_t local_player_index, int16_t slot, int16_t string_index,
                                            uint8_t from_scenario_names); // 0x4ae0b0, blam-cc: EAX local_player_index, ESI slot
extern void hud_set_message_icon_argument(int16_t local_player_index, int16_t slot,
                                          const hud_messaging_information *information); // 0x4ae050, blam-cc: EAX local_player_index, ESI slot
extern void hud_set_action_text_shown(int16_t local_player_index, uint8_t shown); // 0x4ae110, blam-cc: EAX local_player_index, BL shown
extern int16_t object_get_hud_text_message_index(datum_index object_index); // 0x4a9b40, blam-cc: EAX
extern uint8_t weapon_hud_ammo_state_is_empty(const weapon_hud_ammo_state *state); // 0x4a9750, blam-cc: EAX
extern uint8_t game_engine_pick_hud_hint(datum_index player_index, int32_t maximum_length, uint16_t *out_text); // 0x463150, blam-cc: ECX player, EAX 0x400
extern void weapon_build_hud_ammo_state(datum_index item_index, weapon_hud_ammo_state *out); // 0x4c29d0, blam-cc: EAX item
extern int16_t unit_count_deployed_weapons(datum_index unit_index); // 0x56d990, blam-cc: EAX
extern int16_t unit_find_next_zone_permitted_weapon_slot(datum_index unit_index, int32_t start_slot, int16_t direction); // 0x56dba0, blam-cc: EAX unit
extern datum_index unit_get_weapon_object_index(datum_index unit_index, int16_t slot_index); // 0x569970, blam-cc: EAX unit, CX slot
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object

static uint8_t *object_get(datum_index object_index)
{
    return (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
}

static uint8_t *object_tag_data(datum_index object_index)
{
    return (uint8_t *)tag_instances[*(datum_index *)object_get(object_index) & 0xffff].data;
}

// The weapon HUD messaging block of a weapon object: Weapon +0x48c is hud_interface.tag_id,
// WeaponHUDInterface +0x13c is messaging_information_sequence_index. NULL when there is none.
static const int16_t *weapon_hud_messaging(const uint8_t *weapon_object)
{
    datum_index weapon_tag = *(const datum_index *)weapon_object;
    datum_index hud = *(datum_index *)((uint8_t *)tag_instances[weapon_tag & 0xffff].data + 0x48c);
    const int16_t *messaging;

    if (hud == (datum_index)-1) {
        return 0;
    }
    messaging = (const int16_t *)((uint8_t *)tag_instances[hud & 0xffff].data + 0x13c);
    return (*messaging == -1) ? 0 : messaging;
}

// blam-cc: player_index -> EDX
void hud_update_interaction_prompt(datum_index player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    int16_t local = current_local_player_index;
    int16_t target_message;

    if (local_player_globals->mode != 0 && p->unit == (datum_index)-1) {
        static const int16_t mode_message[4] = { 0xb, 0xa, 0x9, 0xc }; // jump table at 0x4aa264
        hud_set_player_message(mode_message[local_player_globals->mode - 1], (uint16_t)local);
        return;
    }

    if (p->interaction_object == (datum_index)-1) {
        target_message = -1;
    } else {
        target_message = *(int16_t *)(object_tag_data(p->interaction_object) + 0x13c);
    }

    switch (p->interaction_type) {
    case 1:
    case 2:
        hud_set_player_message(0, (uint16_t)local);
        hud_set_message_string_argument(local, 0, target_message, 0);
        return;

    case 3: {
        uint8_t *unit_object = object_get(p->unit);
        hud_set_player_message(7, (uint16_t)local);
        hud_set_message_string_argument(local, 0,
            object_get_hud_text_message_index(*(datum_index *)(unit_object + 0x11c)), 0); // parent_object
        return;
    }

    case 5: {
        uint8_t *unit_object = object_get(p->unit);
        hud_set_player_message(1, (uint16_t)local);
        hud_set_message_string_argument(local, 0,
            object_get_hud_text_message_index(*(datum_index *)(unit_object + 0x318)), 0); // equipment_object_index
        hud_set_message_string_argument(local, 1, target_message, 0);
        return;
    }

    case 6:
    case 7: {
        uint8_t *weapon = (uint8_t *)object_try_and_get(p->interaction_object, 4);
        const int16_t *messaging;
        if (weapon == 0) {
            return;
        }
        messaging = weapon_hud_messaging(weapon);
        hud_set_player_message((p->interaction_type == 6) ? 4 : 0, (uint16_t)local);
        if (messaging == 0) {
            hud_set_message_string_argument(local, 0, target_message, 0);
        } else {
            hud_set_message_icon_argument(local, 0, (const hud_messaging_information *)messaging);
        }
        return;
    }

    case 8:
    case 9: {
        uint8_t *seats;
        int16_t seat_message;
        hud_set_player_message(6, (uint16_t)local);
        seats = *(uint8_t **)(object_tag_data(p->interaction_object) + 0x2e8); // Unit seats.pointer
        seat_message = *(int16_t *)(seats + p->interaction_seat * 0x11c + 0xec);
        hud_set_message_string_argument(local, 0, seat_message, 0);
        hud_set_message_string_argument(local, 1, target_message, 0);
        return;
    }

    case 10: {
        uint16_t vehicle_string = *(uint16_t *)(object_get(p->interaction_object) + 0x218);
        if (vehicle_string != 0xffff) {
            hud_set_player_message(3, (uint16_t)local);
            hud_set_message_string_argument(local, 0, (int16_t)vehicle_string, 1);
        } else {
            hud_set_player_message(2, (uint16_t)local);
            hud_set_message_string_argument(local, 0, target_message, 0);
        }
        return;
    }

    case 11:
        hud_set_player_message(8, (uint16_t)local);
        hud_set_message_string_argument(local, 0, object_get_hud_text_message_index(p->interaction_object), 0);
        return;

    default:
        break;
    }

    {
        uint16_t hint_text[0x400];
        if (game_engine_pick_hud_hint(player_index, 0x400, hint_text)) {
            hud_player_messaging_state *msg = &hud_messaging->players[0] + local;
            hud_set_action_text_shown(local, 1);
            wcsncpy((wchar_t *)msg->action_text, (const wchar_t *)hint_text, 0xff);
            msg->action_text[0xff] = 0;
            return;
        }
    }

    if (p->unit == (datum_index)-1) {
        // hud_set_action_text_shown(local, 0) inlined
        hud_player_messaging_state *msg = &hud_messaging->players[0] + local;
        msg->prompt_changed = msg->prompt_changed | (msg->message_shown != 0);
        msg->message_shown = 0;
        msg->message = 0;
        msg->message_shown_copy = 0;
        return;
    }

    {
        datum_index unit_index = p->unit;
        uint8_t *unit_object = object_get(unit_index);
        unit_data *unit = (unit_data *)(unit_object + k_unit_data_offset);
        datum_index current_weapon = unit_get_weapon_object_index(unit_index, unit->current_weapon_index);
        datum_index parent = *(datum_index *)(unit_object + 0x11c);
        uint8_t can_switch = 1;
        weapon_hud_ammo_state ammo;

        if (parent != (datum_index)-1 && unit->vehicle_seat_index != -1) {
            uint8_t *seats = *(uint8_t **)(object_tag_data(parent) + 0x2e8);
            can_switch = ((seats[unit->vehicle_seat_index * 0x11c] & 0xc) == 0);
        }

        if (current_weapon != (datum_index)-1 && can_switch) {
            weapon_build_hud_ammo_state(current_weapon, &ammo);
            if (weapon_hud_ammo_state_is_empty(&ammo)) {
                int16_t slot = unit->current_weapon_index;
                int16_t remaining = unit_count_deployed_weapons(unit_index);
                datum_index candidate;

                for (;;) {
                    slot = unit_find_next_zone_permitted_weapon_slot(unit_index, slot, 1);
                    unit_index = p->unit;
                    candidate = unit_get_weapon_object_index(unit_index, slot);
                    weapon_build_hud_ammo_state(candidate, &ammo);
                    if (!weapon_hud_ammo_state_is_empty(&ammo)) {
                        break; // inlined test at 0x4aa159
                    }
                    if (candidate == current_weapon) {
                        break;
                    }
                    remaining = remaining - 1;
                    if (remaining != 0) {
                        break;
                    }
                }

                if (!weapon_hud_ammo_state_is_empty(&ammo) && candidate != current_weapon) {
                    uint8_t *weapon;
                    const int16_t *messaging;
                    hud_set_player_message(5, (uint16_t)local);
                    weapon = (uint8_t *)object_try_and_get(candidate, 4);
                    if (weapon != 0) {
                        messaging = weapon_hud_messaging(weapon);
                        if (messaging != 0) {
                            hud_set_message_icon_argument(local, 0, (const hud_messaging_information *)messaging);
                        } else {
                            hud_set_message_string_argument(local, 0, target_message, 0);
                        }
                        return;
                    }
                }
            }
        }
    }

    hud_set_action_text_shown(local, 0);
}

#if 0
Original Ghidra decompilation (0x4a9b80):

void FUN_004a9b80(void)

{
  uint uVar1;
  char cVar2;
  undefined2 uVar3;
  short sVar4;
  int iVar5;
  undefined4 uVar6;
  uint *puVar7;
  short *psVar8;
  uint in_EDX;
  int iVar9;
  int iVar10;
  bool bVar11;
  undefined1 local_820 [4];
  float local_81c;
  short local_812;
  short local_810;
  short local_80e;
  wchar_t local_800 [1024];

  iVar5 = DAT_0087bc14;
  iVar10 = DAT_008603b0;
  iVar9 = (in_EDX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  if ((*(short *)(DAT_0087a478 + 0x14) != 0) && (*(int *)(iVar9 + 0x34) == -1)) {
                    /* WARNING: Could not recover jumptable at 0x004a9bbe. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    (*(code *)(&PTR_LAB_004aa264)[*(short *)(DAT_0087a478 + 0x14) + -1])();
    return;
  }
  uVar1 = *(uint *)(iVar9 + 0x24);
  if (uVar1 == 0xffffffff) {
    uVar3 = 0xffff;
  }
  else {
    uVar3 = *(undefined2 *)
             (*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc) &
                       0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x13c);
  }
  switch(*(undefined2 *)(iVar9 + 0x28)) {
  case 1:
  case 2:
    FUN_004adfc0(DAT_007c3108);
    FUN_004ae0b0(uVar3,0);
    return;
  case 3:
    FUN_004adfc0(DAT_007c3108);
    uVar6 = 0;
    uVar3 = FUN_004a9b40(0);
    goto LAB_004aa22f;
  default:
    cVar2 = FUN_00463150(local_800);
    if (cVar2 != '\0') {
      FUN_004ae110();
      iVar10 = DAT_007c3108 * 0x460 + DAT_006b3a40;
      _wcsncpy((wchar_t *)(iVar10 + 0x230),local_800,0xff);
      *(undefined2 *)(iVar10 + 0x42e) = 0;
      return;
    }
    uVar1 = *(uint *)(iVar9 + 0x34);
    if (uVar1 == 0xffffffff) {
      iVar10 = DAT_007c3108 * 0x460;
      iVar5 = iVar10 + DAT_006b3a40;
      *(byte *)(iVar5 + 0x45e) =
           *(byte *)(iVar10 + 0x45e + DAT_006b3a40) |
           *(char *)(iVar10 + 0x458 + DAT_006b3a40) != '\0';
      *(undefined1 *)(iVar5 + 0x458) = 0;
      *(undefined4 *)(iVar5 + 0x454) = 0;
      *(undefined1 *)(iVar5 + 0x45f) = 0;
      return;
    }
    iVar10 = *(int *)(DAT_008603b0 + 0x34);
    iVar9 = unit_get_weapon_object_index();
    iVar5 = *(int *)(iVar10 + 8 + (uVar1 & 0xffff) * 0xc);
    bVar11 = true;
    if ((*(uint *)(iVar5 + 0x11c) != 0xffffffff) && (*(short *)(iVar5 + 0x2f0) != -1)) {
      if ((*(byte *)(*(short *)(iVar5 + 0x2f0) * 0x11c +
                    *(int *)(*(int *)((**(uint **)(iVar10 + 8 +
                                                  (*(uint *)(iVar5 + 0x11c) & 0xffff) * 0xc) &
                                      0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8)) & 0xc) == 0) {
        bVar11 = true;
      }
      else {
        bVar11 = false;
      }
    }
    if ((iVar9 != -1) && (bVar11)) {
      FUN_004c29d0(local_820);
      cVar2 = FUN_004a9750();
      if (cVar2 != '\0') {
        uVar6 = CONCAT22((short)((uint)iVar10 >> 0x10),*(undefined2 *)(iVar5 + 0x2f2));
        sVar4 = FUN_0056d990();
        while( true ) {
          uVar6 = FUN_0056dba0(uVar6,1);
          iVar10 = unit_get_weapon_object_index();
          FUN_004c29d0(local_820);
          if ((((local_810 == 0) || (local_812 != 0)) || (local_80e != 0)) && (local_81c != 1.0))
          break;
          if ((iVar10 == iVar9) || (bVar11 = sVar4 != 1, sVar4 = 0, bVar11)) break;
        }
        cVar2 = FUN_004a9750();
        if ((cVar2 == '\0') && (iVar10 != iVar9)) {
          FUN_004adfc0(DAT_007c3108);
          puVar7 = (uint *)object_try_and_get(4);
          if (puVar7 != (uint *)0x0) {
            uVar1 = *(uint *)(*(int *)((*puVar7 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x48c);
            if ((uVar1 != 0xffffffff) &&
               (psVar8 = (short *)(*(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x13c),
               *psVar8 != -1)) {
              FUN_004ae050(psVar8);
              return;
            }
            uVar6 = 0;
            goto LAB_004aa22f;
          }
        }
      }
    }
    FUN_004ae110();
    break;
  case 5:
    FUN_004adfc0(DAT_007c3108);
    uVar6 = FUN_004a9b40(0);
    FUN_004ae0b0(uVar6);
    FUN_004ae0b0(uVar3,0);
    return;
  case 6:
    puVar7 = (uint *)object_try_and_get(4);
    if (puVar7 != (uint *)0x0) {
      uVar1 = *(uint *)(*(int *)((*puVar7 & 0xffff) * 0x20 + 0x14 + iVar5) + 0x48c);
      psVar8 = (short *)0x0;
      if ((uVar1 != 0xffffffff) &&
         (psVar8 = (short *)(*(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + iVar5) + 0x13c),
         *psVar8 == -1)) {
        psVar8 = (short *)0x0;
      }
      FUN_004adfc0(DAT_007c3108);
joined_r0x004a9eda:
      if (psVar8 == (short *)0x0) {
        FUN_004ae0b0(uVar3,0);
        return;
      }
      FUN_004ae050(psVar8);
      return;
    }
    break;
  case 7:
    puVar7 = (uint *)object_try_and_get(4);
    if (puVar7 != (uint *)0x0) {
      uVar1 = *(uint *)(*(int *)((*puVar7 & 0xffff) * 0x20 + 0x14 + iVar5) + 0x48c);
      psVar8 = (short *)0x0;
      if ((uVar1 != 0xffffffff) &&
         (psVar8 = (short *)(*(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + iVar5) + 0x13c),
         *psVar8 == -1)) {
        psVar8 = (short *)0x0;
      }
      FUN_004adfc0(DAT_007c3108);
      goto joined_r0x004a9eda;
    }
    break;
  case 8:
  case 9:
    FUN_004adfc0(DAT_007c3108);
    FUN_004ae0b0(*(undefined2 *)
                  (*(int *)(*(int *)((**(uint **)(*(int *)(iVar10 + 0x34) + 8 +
                                                 (*(uint *)(iVar9 + 0x24) & 0xffff) * 0xc) & 0xffff)
                                     * 0x20 + 0x14 + iVar5) + 0x2e8) + 0xec +
                  *(short *)(iVar9 + 0x2a) * 0x11c),0);
    FUN_004ae0b0(uVar3,0);
    return;
  case 10:
    iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
    if (*(short *)(iVar10 + 0x218) == -1) {
      FUN_004adfc0(DAT_007c3108);
      FUN_004ae0b0(uVar3,0);
      return;
    }
    FUN_004adfc0(DAT_007c3108);
    FUN_004ae0b0(*(undefined2 *)(iVar10 + 0x218),1);
    return;
  case 0xb:
    FUN_004adfc0(DAT_007c3108);
    uVar6 = 0;
    uVar3 = FUN_004a9b40(0);
LAB_004aa22f:
    FUN_004ae0b0(uVar3,uVar6);
    return;
  }
  return;
}
#endif
