// weapon_trigger_begin_reload  (Ghidra: weapon_trigger_begin_reload, already named)
// address 0x4c35b0, size 510 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: types/items.h weapon_magazine_state (state/state_ticks/state_ticks_total,
//   rounds_unloaded/rounds_loaded), weapon_data.predicted_rounds_unloaded/_loaded,
//   weapon_state (_weapon_state_reload_primary=5/_secondary=6), weapon_flags
//   (_weapon_ammo_prediction_pending_bit); types/tags.h WeaponMagazine.rounds_loaded_maximum
//   (0x0a), Weapon.weapon_type (0x4e2); global 0x00719720 network_game_mode (2 = host).
// register convention: item index, magazine/trigger index and a client/predicted flag are all
// Ghidra-recognized __cdecl parameters.
// blam-cc: stack -> (item_index, magazine_index, is_client_predicted)
// UNSURE: weapon_play_trigger_tag_effect and weapon_get_first_person_animation_time are called
// here with literal (0,0)/(0,mode) arguments in the decompilation; item_index and the animation
// index are threaded through as this module's own convention requires, but the true animation
// index (CX at the second call) is not recoverable -- passed as magazine_index here since that
// is the only in-scope value that plausibly maps to "which reload animation".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t network_game_mode;   // 0x00719720

extern int32_t weapon_triggers_idle(datum_index item_index); // 0x4c30c0
extern void weapon_trigger_effect_clear(datum_index item_index, int16_t trigger_index); // 0x4c3e40
extern void weapon_notify_reload_begin(datum_index item_index, int16_t magazine_index); // 0x4c3470
extern int32_t weapon_set_state(datum_index item_index, int16_t new_state, int8_t force); // 0x4c5670
extern uint32_t weapon_play_trigger_tag_effect(datum_index item_index, datum_index tag_id, int32_t slot, int32_t sub_index); // 0x4c47d0
extern void weapon_action_notify_for_weapon(datum_index weapon_index, int32_t action_code); // 0x492790, EAX, EDI
extern int16_t weapon_get_first_person_animation_time(datum_index item_index, int16_t animation_index,
    int16_t category, int16_t mode); // 0x4c2f80

// Starts loading a fresh round into a weapon trigger's chamber/magazine. When called
// client-side with is_client_predicted, first copies the client's own predicted round counts
// into the magazine before deciding whether there is room to reload; when called host-side in a
// network game, notifies observers that the reload began.
void weapon_trigger_begin_reload(datum_index item_index, int16_t magazine_index, int8_t is_client_predicted)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    weapon_magazine_state *magazine;
    WeaponMagazine *magazine_tag;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    magazine = &wd->magazines[magazine_index];
    magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + magazine_index;

    if (item_obj->network_role == 1 &&
        (magazine->state == 0 || magazine->state == 2) &&
        weapon_triggers_idle(item_index) == 0 &&
        wd->triggers[0].effect_state == _weapon_trigger_effect_out_of_ammo) {
        weapon_trigger_effect_clear(item_index, 0);
    }

    if ((magazine->state == 0 || magazine->state == 2) &&
        wd->triggers[0].effect_state == 0 && wd->triggers[1].effect_state == 0 && wd->state == 0) {
        if (item_obj->network_role == 1 && is_client_predicted == 1) {
            magazine->rounds_unloaded = wd->predicted_rounds_unloaded[magazine_index];
            magazine->rounds_loaded = wd->predicted_rounds_loaded[magazine_index];
        }

        if (magazine->rounds_unloaded > 0 && magazine->rounds_loaded < magazine_tag->rounds_loaded_maximum) {
            int16_t mode = -1;
            int16_t ticks;

            if (is_client_predicted == 1 && item_obj->network_role == 0 && network_game_mode == 2) {
                weapon_notify_reload_begin(item_index, magazine_index);
            }
            weapon_set_state(item_index, magazine_index + 5, 0);
            weapon_play_trigger_tag_effect(item_index, *(datum_index *)&magazine_tag->reloading_effect.tag_id, 0, 0); // UNSURE:
                // tag_id (EDI) placeholder, see weapon_play_trigger_tag_effect.c
            // FIXED (objdump 0x4c3724..0x4c3739): EAX = the weapon, EDI = 9 + (rounds_loaded != 0). The draft passed
            //   nothing, so the first-person reload action ran with a garbage player index and action code.
            weapon_action_notify_for_weapon(item_index, magazine->rounds_loaded != 0 ? 10 : 9);

            if (weapon_tag->weapon_type == 1) {
                int16_t remaining = magazine_tag->rounds_loaded_maximum - magazine->rounds_loaded;
                mode = (remaining == 1) ? (is_client_predicted == 0 ? 1 : 2)
                                        : (is_client_predicted == 0 ? -1 : 0);
            }

            magazine->state = _weapon_magazine_reloading;
            ticks = weapon_get_first_person_animation_time(item_index, magazine_index, 0, mode);
            magazine->state_ticks = ticks;
            magazine->state_ticks_total = ticks;
        }
        wd->flags = wd->flags & ~(uint32_t)_weapon_ammo_prediction_pending_bit;
    }
}

#if 0
Original Ghidra decompilation (0x4c35b0):

void weapon_trigger_begin_reload(uint param_1,int param_2,char param_3)

{
  uint *puVar1;
  uint *puVar2;
  char cVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;

  iVar5 = (int)(short)param_2;
  iVar8 = (param_1 & 0xffff) * 0xc;
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
  puVar1 = puVar2 + iVar5 * 3 + 0xac;
  iVar6 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar7 = iVar5 * 0x70 + *(int *)(iVar6 + 0x4f4);
  if ((puVar2[1] == 1) &&
     (((((short)*puVar1 == 0 || ((short)*puVar1 == 2)) && (cVar3 = FUN_004c30c0(), cVar3 == '\0'))
      && (*(char *)((int)puVar2 + 0x261) == '\a')))) {
    FUN_004c3e40(0);
  }
  if ((((short)*puVar1 == 0) || ((short)*puVar1 == 2)) &&
     ((iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8), *(char *)(iVar8 + 0x261) == '\0'
      && ((*(char *)(iVar8 + 0x289) == '\0' && (*(char *)(iVar8 + 0x238) == '\0')))))) {
    if ((puVar2[1] == 1) && (param_3 == '\x01')) {
      *(short *)((int)puVar1 + 6) = *(short *)((int)puVar2 + iVar5 * 2 + 0x2d4);
      *(short *)(puVar1 + 2) = *(short *)((int)puVar2 + iVar5 * 2 + 0x2d8);
    }
    if ((0 < *(short *)((int)puVar1 + 6)) && ((short)puVar1[2] < *(short *)(iVar7 + 10))) {
      iVar5 = -1;
      if ((param_3 == '\x01') && ((puVar2[1] == 0 && (DAT_00719720 == 2)))) {
        FUN_004c3470();
      }
      weapon_set_state(param_2 + 5,0);
      weapon_play_trigger_tag_effect(0,0);
      FUN_00492790();
      if (*(short *)(iVar6 + 0x4e2) == 1) {
        iVar6 = (int)*(short *)(iVar7 + 10);
        if (param_3 == '\0') {
          iVar5 = (-(uint)(iVar6 - (short)puVar1[2] != 1) & 0xfffffffe) + 1;
        }
        else {
          iVar5 = (-(uint)(iVar6 - (short)puVar1[2] != 1) & 0xfffffffe) + 2;
        }
      }
      *(short *)puVar1 = 1;
      sVar4 = FUN_004c2f80(0,iVar5);
      *(short *)((int)puVar1 + 2) = sVar4;
      *(short *)(puVar1 + 1) = sVar4;
    }
    puVar2[0x8b] = puVar2[0x8b] & 0xfffffff7;
  }
  return;
}
#endif
