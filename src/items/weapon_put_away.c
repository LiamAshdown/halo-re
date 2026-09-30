// weapon_put_away  (Ghidra: FUN_004c28f0; named from out/phase4/items_functions.md, "Stops a
// weapon's current action animation/sound and cancels any associated looping effect")
// address 0x4c28f0, size 154 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/items.h weapon_data.control_flags (0x230), .overheat_effect_handle (0x2cc);
//   types/objects.h object_header/object_data.
// register convention: item index in ESI; "force" flag in AL.
// blam-cc: ESI -> item_index, AL -> force
// FIXED (register inputs, objdump): ESI (read at 0x4c28fc, "mov eax,esi") is item_index; the
// note previously said "reg -> item_index" which the checker can't parse as a register, so ESI
// looked unclaimed even though the body already used item_index correctly.
// UNSURE: weapon_set_state's return here is treated as a bool per its own signature; the
// original decompiles it as returning through AL/EAX truncated to char.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "fn_items.h"
#include "fn_interface.h"

extern data_array *object_data; // 0x008603b0

extern int8_t weapon_has_active_state(datum_index item_index); // 0x4c3070

extern void weapon_reset_triggers(datum_index item_index); // 0x4c4b50
extern void effect_delete(datum_index handle); // 0x450be0
extern uint32_t local_player_index_for_weapon(datum_index item_index); // 0x494010, outside this module, UNSURE signature
extern void first_person_weapon_process_action(uint32_t handle, int32_t action); // 0x4940f0


// Puts a weapon away: refuses (returns 0) if it has active trigger/reload state and the request
// isn't forced, otherwise clears control_flags, resets every trigger, deletes any overheat
// particle system and plays the put-away first-person action.
int32_t weapon_put_away(datum_index item_index, int8_t force)
{
    object *item_obj;
    weapon_data *wd;
    uint32_t action_handle;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    if (force == 0 && weapon_has_active_state(item_index) != 0) {
        return 0;
    }
    if (weapon_set_state(item_index, _weapon_state_put_away, 0) == 0) {
        return 0;
    }

    wd->control_flags = 0;
    weapon_reset_triggers(item_index);

    if (wd->overheat_effect_handle != (datum_index)0xffffffff) {
        effect_delete(wd->overheat_effect_handle);
        wd->overheat_effect_handle = (datum_index)0xffffffff;
    }

    action_handle = local_player_index_for_weapon(item_index); // the argument is a plain stack push that
        // Ghidra dropped here but recovered in weapon_fire_trigger.c; objdump shows
        // `push ebx` / `push esi` immediately before the call
    first_person_weapon_process_action(action_handle, 0x0b);
    if ((int16_t)action_handle == -1) {
        hud_play_pickup_notification(item_index, 0xb); // FIXED: EBX = the weapon, EAX = 0xb (0x4c2974)
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x4c28f0):

undefined4 FUN_004c28f0(void)

{
  int iVar1;
  char in_AL;
  char cVar2;
  undefined4 uVar3;
  uint unaff_ESI;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_ESI & 0xffff) * 0xc);
  if ((in_AL == '\0') && (cVar2 = item_has_active_state(), cVar2 != '\0')) {
    return 0;
  }
  cVar2 = weapon_set_state(10);
  if (cVar2 == '\0') {
    return 0;
  }
  *(undefined2 *)(iVar1 + 0x230) = 0;
  FUN_004c4b50();
  if (*(int *)(iVar1 + 0x2cc) != -1) {
    particle_system_delete_450be0(*(int *)(iVar1 + 0x2cc));
    *(undefined4 *)(iVar1 + 0x2cc) = 0xffffffff;
  }
  uVar3 = FUN_00494010();
  first_person_weapon_process_action(uVar3,0xb);
  if ((short)uVar3 == -1) {
    FUN_00492990();
  }
  return 1;
}
#endif
