// local_player_set_controlled_unit  (Ghidra: local_player_set_controlled_unit, already named)
// address 0x474fc0, size 201 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: out/phase4/game_functions.md ("Binds a new unit as the controlled object for a
//   local player, updating both the unit's owner back-reference and the player's unit-index
//   field"); types/units.h unit_data::controlling_player (+0x218 absolute); types/game.h
//   local_player_control (player_control_globals->local_players[i].unit at +0x10+i*0x40),
//   player_globals::local_players (+0x04), player::unit/previous_unit (+0x34/+0x38);
//   game_engine_init_player_look_state_from_object.c (this module) for the tail-call signature.
// objdump -d -M intel --start-address=0x474fc0 --stop-address=0x475090 bin/halo.exe confirms
// every register, including unit_refresh_targeting_flag_and_weapons's CL flag (0 when detaching the old unit, 1 when
// attaching the new one).
// register convention: ESI -> new_unit, DI -> local_player_index.
//   // blam-cc: ESI -> new_unit, DI -> local_player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern data_array *object_data;                             // 0x008603b0
extern player_globals *local_player_globals;                // 0x0087a478
extern data_array *player_data;                              // 0x0087a480

extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_handle, uint8_t attaching); // 0x569bf0, units module,
    // not in this batch; blam-cc: stack -> unit_handle, CL -> attaching
extern void game_engine_init_player_look_state_from_object(datum_index unit, int16_t local_player_index); // 0x470e80

// Detaches the local player slot's currently controlled unit (clears its controlling_player and
// notifies unit_refresh_targeting_flag_and_weapons), then attaches `new_unit` as this local player's controlled unit
// (setting its controlling_player to the player bound to this slot, or the wildcard if the slot
// index is out of range). Finally sets that player's unit/previous_unit fields and tail-calls
// game_engine_init_player_look_state_from_object to (re)seed the look state.
void local_player_set_controlled_unit(datum_index new_unit, int16_t local_player_index)
    // blam-cc: ESI -> new_unit, DI -> local_player_index
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    datum_index old_unit = look->unit;
    datum_index owner;
    object *obj;
    unit_data *unit;
    player *plr;

    if (old_unit != (datum_index)-1) {
        obj = ((object_header *)object_data->data)[old_unit & 0xffff].data;
        unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        unit->controlling_player = (datum_index)-1;
        unit_refresh_targeting_flag_and_weapons(old_unit, 0);
    }

    if (new_unit != (datum_index)-1) {
        obj = ((object_header *)object_data->data)[new_unit & 0xffff].data;
        unit_refresh_targeting_flag_and_weapons(new_unit, 1);
        owner = (datum_index)-1;
        if (local_player_index != -1 && local_player_index <= 0) {
            owner = local_player_globals->local_players[local_player_index];
        }
        unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        unit->controlling_player = owner;
    }

    owner = (datum_index)-1;
    if (local_player_index != -1 && local_player_index <= 0) {
        owner = local_player_globals->local_players[local_player_index];
    }
    plr = (player *)((uint8_t *)player_data->data + (owner & 0xffff) * sizeof(player));
    plr->unit = new_unit;
    plr->previous_unit = (datum_index)-1;

    game_engine_init_player_look_state_from_object(new_unit, local_player_index);
}

#if 0
Original Ghidra decompilation (0x474fc0), from tools/pack.py 0x474fc0:

void local_player_set_controlled_unit(void)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint unaff_ESI;
  short unaff_DI;

  iVar3 = DAT_008603b0;
  iVar4 = (int)unaff_DI;
  uVar2 = *(uint *)(iVar4 * 0x40 + 0x10 + DAT_006b145c);
  if (uVar2 != 0xffffffff) {
    *(undefined4 *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 0x218) =
         0xffffffff;
    FUN_00569bf0(uVar2);
  }
  if (unaff_ESI != 0xffffffff) {
    iVar3 = *(int *)(*(int *)(iVar3 + 0x34) + 8 + (unaff_ESI & 0xffff) * 0xc);
    FUN_00569bf0();
    if ((unaff_DI == -1) || (0 < unaff_DI)) {
      uVar1 = 0xffffffff;
    }
    else {
      uVar1 = *(undefined4 *)(DAT_0087a478 + 4 + iVar4 * 4);
    }
    *(undefined4 *)(iVar3 + 0x218) = uVar1;
  }
  if ((unaff_DI == -1) || (0 < unaff_DI)) {
    uVar2 = 0xffffffff;
  }
  else {
    uVar2 = *(uint *)(DAT_0087a478 + 4 + iVar4 * 4);
  }
  iVar3 = (uVar2 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  *(uint *)(iVar3 + 0x34) = unaff_ESI;
  *(undefined4 *)(iVar3 + 0x38) = 0xffffffff;
  game_engine_init_player_look_state_from_object();
  return;
}
#endif
