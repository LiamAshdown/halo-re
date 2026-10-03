// weapon_trigger_become_charged  (Ghidra: FUN_004c3bc0; named per types/items.h
// weapon_trigger_effect_state comment block: "_weapon_trigger_effect_charged = 3, // 0x4c3bc0")
// address 0x4c3bc0, size 156 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against objdump 0x4c3bc0..0x4c3c5b)
// evidence: types/items.h weapon_trigger_effect_state (_weapon_trigger_effect_charged=3),
//   weapon_state (_weapon_state_charged_primary=7/_secondary=8).
// register convention: item index in EAX; trigger index is a Ghidra-recognized parameter.
// blam-cc: EAX -> item_index, stack -> trigger_index
// UNSURE: the value truncated by __ftol() into effect_state_ticks is not shown in the
// decompilation; almost certainly WeaponTrigger.charged_time * 30 ticks/second (see
// weapon_trigger_get_charge_fraction.c, which divides effect_state_ticks/30 by charged_time),
// rendered that way here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int32_t weapon_set_state(datum_index item_index, int16_t new_state, int8_t force); // 0x4c5670
extern uint32_t local_player_index_for_weapon(datum_index item_index); // 0x494010, outside this module, UNSURE signature
extern void first_person_weapon_process_action(uint32_t handle, int32_t action); // 0x4940f0
extern void hud_play_pickup_notification(uint32_t object_or_slot_index, int16_t item_type_code); // 0x492990, EBX, EAX

// Transitions a trigger into the "charged" effect state and its matching weapon_state, and
// starts the first-person charged-loop action.
void weapon_trigger_become_charged(datum_index item_index, int16_t trigger_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;
    uint32_t action_handle;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    wd->triggers[trigger_index].effect_state_ticks = (int16_t)(tag_trigger->charged_time * 30.0f);
    wd->triggers[trigger_index].effect_state = _weapon_trigger_effect_charged;

    weapon_set_state(item_index, trigger_index + 7, 1);
    action_handle = local_player_index_for_weapon(item_index); // the argument is a plain stack push that
        // Ghidra dropped here but recovered in weapon_fire_trigger.c; objdump shows
        // `push ebx` / `push esi` immediately before the call
    first_person_weapon_process_action(action_handle, 0x0e);
    if ((int16_t)action_handle == -1) {
        hud_play_pickup_notification(item_index, 0xe); // FIXED: EBX = the weapon, EAX = 0xe (0x4c3c4d)
    }
}

#if 0
Original Ghidra decompilation (0x4c3bc0):

void FUN_004c3bc0(int param_1)

{
  int iVar1;
  undefined2 uVar2;
  uint in_EAX;
  undefined4 uVar3;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar2 = __ftol();
  iVar1 = iVar1 + (short)param_1 * 0x28;
  *(undefined2 *)(iVar1 + 0x262) = uVar2;
  *(undefined1 *)(iVar1 + 0x261) = 3;
  weapon_set_state(param_1 + 7,1);
  uVar3 = FUN_00494010();
  first_person_weapon_process_action(uVar3,0xe);
  if ((short)uVar3 == -1) {
    FUN_00492990();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
