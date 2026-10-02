// player_kill_and_release_unit  (Ghidra: FUN_00476250; named per this rewrite)
// address 0x476250, size 157 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Handles the loss of a player's controlled unit,
//   clearing related state and triggering the post-unit-change reset"); types/objects.h
//   object::body_vitality (+0xe0); types/game.h player::respawn_timer (+0x2c);
//   game_engine_attribute_player_death.c / player_reset_after_unit_change.c (this module).
// objdump -d -M intel --start-address=0x476250 --stop-address=0x4762e8 bin/halo.exe confirms
// every register, including that game_engine_attribute_player_death's victim_unit (EAX) is the
// player's own unit handle, and unit_exit_vehicle_seat takes the PLAYER index in EAX, not a unit
// handle.
// register convention: EBX -> player_index, stack -> respawn_timer_override.
//   // blam-cc: EBX -> player_index, stack -> respawn_timer_override
//
// UNSURE: unit_exit_vehicle_seat (0x568120, units module, not in this batch) is named only from
// its game_functions.md-adjacent context; its taking a player index rather than a unit handle is
// read directly off the disassembly but not independently confirmed inside that function itself.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value;    // 0x0087aa10

extern void game_engine_attribute_player_death(datum_index victim_unit, datum_index killer,
    datum_index death_object, int32_t killer_team, char credit_kills); // 0x46ff00
extern void object_set_health_frozen_flag(uint32_t object_index); // 0x4eda20
extern void unit_exit_vehicle_seat(uint32_t player_index); // 0x568120, UNSURE: see header
extern void player_reset_after_unit_change(uint32_t player_index); // this batch, 0x474e10

// If player_index is valid and its player has a unit: attributes a death for that unit (no
// credited kill) unless a multiplayer game has already ended, optionally overwrites
// player::respawn_timer, zeroes the unit's body_vitality, freezes its health, exits it from any
// vehicle seat, and finally runs player_reset_after_unit_change.
void player_kill_and_release_unit(uint32_t player_index, int32_t respawn_timer_override)
    // blam-cc: EBX -> player_index, stack -> respawn_timer_override
{
    player *plr;
    datum_index unit_handle;

    if (player_index == (uint32_t)-1) {
        return;
    }

    plr = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    unit_handle = plr->unit;
    if (unit_handle == (datum_index)-1) {
        return;
    }

    if (current_game_engine == 0 || game_engine_state_value == _game_engine_state_not_started) {
        game_engine_attribute_player_death(unit_handle, (datum_index)-1, (datum_index)-1, -1, 0);
    }

    if (respawn_timer_override != 0) {
        plr->respawn_timer = respawn_timer_override;
    }

    {
        object *unit_obj = ((object_header *)object_data->data)[unit_handle & 0xffff].data;
        unit_obj->body_vitality = 0.0f;
    }
    object_set_health_frozen_flag(unit_handle);
    unit_exit_vehicle_seat(player_index);
    player_reset_after_unit_change(player_index);
}

#if 0
Original Ghidra decompilation (0x476250), from tools/pack.py 0x476250:

void FUN_00476250(int param_1)

{
  int iVar1;
  uint uVar2;
  uint unaff_EBX;
  int iVar3;

  if (unaff_EBX != 0xffffffff) {
    iVar1 = *(int *)(DAT_0087a480 + 0x34);
    iVar3 = (unaff_EBX & 0xffff) * 0x200;
    uVar2 = *(uint *)(iVar3 + 0x34 + iVar1);
    if (uVar2 != 0xffffffff) {
      if ((DAT_006f1d20 == 0) || (DAT_0087aa10 == 0)) {
        game_engine_attribute_player_death(0xffffffff,0xffffffff,0xffffffff,0);
      }
      if (param_1 != 0) {
        *(int *)(iVar3 + iVar1 + 0x2c) = param_1;
      }
      *(undefined4 *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 0xe0) =
           0;
      object_set_health_frozen_flag();
      unit_exit_vehicle_seat();
      FUN_00474e10();
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
