// weapon_action_notify_for_weapon  (Ghidra: FUN_00492790, unnamed)
// address 0x492790, size 38 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Routes a weapon action event keyed by weapon
// object: applies it if the weapon belongs to a local player, otherwise plays a fallback
// notification." Disassembled (objdump bin/halo.exe 0x492790..0x4927b5): Ghidra's signature
// shows no parameters at all, but EAX (weapon_index) and EDI (action_code) are both genuinely
// live incoming register arguments -- EDI is never written before its first use (pushed
// straight through to first_person_weapon_process_action and later moved into EAX for the
// hud_play_pickup_notification call).
// register convention: weapon_index in EAX, action_code in EDI (both unrecognized by Ghidra).
// // blam-cc: weapon_index=EAX, action_code=EDI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t local_player_index_for_weapon(datum_index weapon_index); // 0x494010, this module
extern void first_person_weapon_process_action(int32_t local_player_index, int32_t action_code); // 0x4940f0, this module
extern void hud_play_pickup_notification(uint32_t object_or_slot_index, int16_t item_type_code); // 0x492990, this module

// Applies a weapon HUD action to weapon_index's local first-person weapon interface if the
// weapon is equipped by a local player; otherwise plays the fallback pickup/HUD notification.
void weapon_action_notify_for_weapon(datum_index weapon_index, int32_t action_code)
{
    int32_t local_player;

    local_player = local_player_index_for_weapon(weapon_index);
    first_person_weapon_process_action(local_player, action_code);

    if (local_player == -1) {
        hud_play_pickup_notification((uint32_t)weapon_index, (int16_t)action_code);
    }
}

#if 0
Original Ghidra decompilation (0x492790):

void FUN_00492790(void)

{
  undefined4 uVar1;

  uVar1 = FUN_00494010();
  first_person_weapon_process_action(uVar1);
  if ((short)uVar1 == -1) {
    FUN_00492990();
  }
  return;
}
#endif
