// player_apply_first_position_update  (Ghidra: FUN_00476cf0; named per this rewrite)
// address 0x476cf0, size 79 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/game_functions.md ("Applies the first position update for a newly
//   spawned local player's unit, choosing a smooth or snap update based on whether the unit is
//   attached to another object"); types/units.h unit_data::controlling_player (+0x218);
//   this batch's player_apply_first_position_update caller
//   (src/game/game_engine_players_update_client.c) and callees (FUN_00477210,
//   apply_remote_player_position_update, apply_remote_player_vehicle_position_update, all this
//   batch).
// objdump -d -M intel --start-address=0x476cf0 --stop-address=0x476d40 bin/halo.exe confirms
// every register, including that ECX (the object handle passed to object_try_and_get) is
// player::unit surviving unchanged from the earlier comparison, and that FUN_00477210 is called
// with the unit's controlling_player (a player handle), not the unit handle itself.
// register convention: ESI -> plr, EDI -> field0.
//   // blam-cc: ESI -> plr, EDI -> field0

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t network_game_mode; // 0x00719720

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t player_unit_has_parent(datum_index player_handle); // this batch, 0x477210, blam-cc: ECX -> player_handle
extern void apply_remote_player_position_update(player *plr, object *unit_obj); // this batch, 0x477350, blam-cc: EAX -> plr, EBX -> unit_obj
extern void apply_remote_player_vehicle_position_update(player *plr, object *unit_obj); // this batch, 0x477490, blam-cc: EAX -> plr, EBX -> unit_obj

// Only while this machine is a network client, and only for a non-local player with a unit:
// revalidates the unit (_object_mask_unit) and stamps field0 into its +0x4bc (see
// game_engine_server_update_player_positions.c, this batch, for the producer side). Then, based
// on whether FUN_00477210 says the unit's controlling player currently has a parent object (e.g.
// boarding or seated), applies either a smooth remote-player position update or the vehicle
// variant.
void player_apply_first_position_update(uint32_t field0, player *plr)
    // blam-cc: ESI -> plr, EDI -> field0
{
    object *unit_obj;

    if (network_game_mode != 1 || plr->local_player_index != -1 || plr->unit == (datum_index)-1) {
        return;
    }

    unit_obj = object_try_and_get(plr->unit, _object_mask_unit);
    if (unit_obj == (object *)0) {
        return;
    }

    {
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        uint8_t seated = player_unit_has_parent(unit->controlling_player);
        *(uint32_t *)((uint8_t *)unit_obj + 0x4bc) = field0;
        if (seated == 0) {
            apply_remote_player_position_update(plr, unit_obj);
        } else {
            apply_remote_player_vehicle_position_update(plr, unit_obj);
        }
    }
}

#if 0
Original Ghidra decompilation (0x476cf0), from tools/pack.py 0x476cf0:

void FUN_00476cf0(void)

{
  char cVar1;
  int iVar2;
  int unaff_ESI;
  undefined4 unaff_EDI;

  if ((((DAT_00719720 == 1) && (*(short *)(unaff_ESI + 2) == -1)) &&
      (*(int *)(unaff_ESI + 0x34) != -1)) && (iVar2 = object_try_and_get(3), iVar2 != 0)) {
    cVar1 = FUN_00477210();
    *(undefined4 *)(iVar2 + 0x4bc) = unaff_EDI;
    if (cVar1 == '\0') {
      apply_remote_player_position_update();
      return;
    }
    apply_remote_player_vehicle_position_update();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
