// weapon_ready  (Ghidra: FUN_004c2840; named from out/phase4/items_functions.md, "Starts an
// item's action animation/sound (e.g. ready/deploy) and its associated cooldown timer")
// address 0x4c2840, size 161 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/items.h weapon_data.action_ticks (0x23a; seeded from
//   weapon_get_first_person_animation_time(0,-1) per the weapon_data comment block);
//   types/objects.h object.flags (_object_changed_bit 0x04000000), object.network_role (0x004).
// register convention: item index in EAX, threaded through to every callee below.
// blam-cc: EAX -> item_index
// UNSURE: local_player_index_for_weapon and hud_play_pickup_notification are outside this module; their true signatures and
// whether they also take item_index implicitly are not established, so they are declared with
// only the arguments Ghidra shows.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void weapon_reset_triggers(datum_index item_index); // 0x4c4b50
extern int32_t weapon_set_state(datum_index item_index, int16_t new_state, int8_t force); // 0x4c5670
extern uint32_t local_player_index_for_weapon(datum_index item_index); // 0x494010, outside this module, UNSURE signature
extern void first_person_weapon_process_action(uint32_t handle, int32_t action); // 0x4940f0
extern void hud_play_pickup_notification(void); // 0x492990, outside this module, UNSURE signature
extern uint32_t weapon_play_trigger_tag_effect(datum_index item_index, datum_index tag_id, int32_t slot, int32_t sub_index); // 0x4c47d0
extern int16_t weapon_get_first_person_animation_time(datum_index item_index, int16_t animation_index,
    int16_t category, int16_t mode); // 0x4c2f80

// Starts a weapon's "ready" state, kicks off its first-person ready animation/sound, and seeds
// the action_ticks cooldown that keeps weapon_update from accepting triggers until it elapses.
void weapon_ready(datum_index item_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    uint32_t action_handle;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    weapon_reset_triggers(item_index);
    weapon_set_state(item_index, _weapon_state_ready, 1);

    action_handle = local_player_index_for_weapon(item_index); // the argument is a plain stack push that
        // Ghidra dropped here but recovered in weapon_fire_trigger.c; objdump shows
        // `push ebx` / `push esi` immediately before the call
    first_person_weapon_process_action(action_handle, 0xc);
    if ((int16_t)action_handle == -1) {
        hud_play_pickup_notification();
    }

    weapon_play_trigger_tag_effect(item_index, *(datum_index *)&weapon_tag->ready_effect.tag_id, 0, 0); // UNSURE:
        // tag_id (EDI) placeholder inferred as Weapon.ready_effect, see
        // weapon_play_trigger_tag_effect.c
    wd->action_ticks = weapon_get_first_person_animation_time(item_index, 0, 0, -1); // UNSURE:
        // animation_index (CX) placeholder, see weapon_get_first_person_animation_time.c

    if (item_obj->network_role == 0) {
        item_obj->flags = item_obj->flags | _object_changed_bit;
    }
}

#if 0
Original Ghidra decompilation (0x4c2840):

void FUN_004c2840(void)

{
  int iVar1;
  undefined2 uVar2;
  uint in_EAX;
  undefined4 uVar3;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  FUN_004c4b50();
  weapon_set_state(9,1);
  uVar3 = FUN_00494010();
  first_person_weapon_process_action(uVar3,0xc);
  if ((short)uVar3 == -1) {
    FUN_00492990();
  }
  weapon_play_trigger_tag_effect(0,0);
  uVar2 = FUN_004c2f80(0,0xffffffff);
  *(undefined2 *)(iVar1 + 0x23a) = uVar2;
  if (*(int *)(iVar1 + 4) == 0) {
    *(uint *)(iVar1 + 0x10) = *(uint *)(iVar1 + 0x10) | 0x4000000;
  }
  return;
}
#endif
