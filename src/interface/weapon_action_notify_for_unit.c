// weapon_action_notify_for_unit  (Ghidra: FUN_00492730, unnamed)
// address 0x492730, size 82 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: out/phase4/interface_functions.md "Routes a weapon action event for a given unit:
// applies it locally if the unit belongs to a local player, otherwise plays a fallback
// notification." Disassembled (objdump bin/halo.exe 0x492730..0x49277e) to pin the exact
// register convention and the value passed to hud_play_pickup_notification.
// register convention: unit_index in EAX (in_EAX), action_code is the one Ghidra-recognized
// stack parameter (param_1). // blam-cc: unit_index=EAX, action_code=stack
// UNSURE: the value passed as hud_play_pickup_notification's object_or_slot_index here is
// unit->current_weapon_index (an inventory slot, 0..3), not an object datum_index -- see that
// file's header note for the full explanation; preserved exactly as the binary does it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0, "objects"

extern int32_t local_player_index_for_unit(datum_index unit_index); // 0x4940a0, this module
extern void first_person_weapon_process_action(int32_t local_player_index, int32_t action_code); // 0x4940f0, this module
extern void hud_play_pickup_notification(uint32_t object_or_slot_index, int16_t item_type_code); // 0x492990, this module

// Applies a weapon HUD action to unit_index's local first-person weapon interface if the unit
// belongs to a local player; otherwise, if the unit has a current weapon, plays the fallback
// pickup/HUD notification instead.
void weapon_action_notify_for_unit(datum_index unit_index, int32_t action_code)
{
    int32_t local_player;
    object_header *header;
    unit_data *u;

    local_player = local_player_index_for_unit(unit_index);
    first_person_weapon_process_action(local_player, action_code);

    if (local_player == -1) {
        header = &((object_header *)object_data->data)[unit_index & 0xffff];
        u = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);
        if (u->current_weapon_index != -1) {
            hud_play_pickup_notification((uint32_t)(uint16_t)u->current_weapon_index, (int16_t)action_code);
        }
    }
}

#if 0
Original Ghidra decompilation (0x492730):

void FUN_00492730(undefined4 param_1)

{
  uint in_EAX;
  undefined4 uVar1;

  uVar1 = FUN_004940a0();
  first_person_weapon_process_action(uVar1,param_1);
  if (((short)uVar1 == -1) &&
     (*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x2f2) !=
      -1)) {
    FUN_00492990();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
