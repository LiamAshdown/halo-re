// game_engine_ctf_player_drop_flag  (Ghidra: FUN_004688b0; named per its summary)
// address 0x4688b0, size 85 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/game_functions.md ("Attaches/updates a flag-carry state on a player's
//   unit, then resets the associated team's flag-return credit"); types/game.h player::unit
//   (0x34), player_data (0x0087a480); unit_dispatch_scripted_event_1b (0x56dcd0, already
//   committed, src/units/) blam-cc stack -> event_byte, ECX -> unit_index; unit_drop_current_
//   weapon (0x56dec0, already committed). Calls game_engine_ctf_reset_team_return_credit
//   (0x468840, this batch), whose own forwarded flag_object_index/position parameters are, like
//   here, never touched by this function's body either and must come from further up the call
//   chain -- modeled the same way.
// register convention: player index in_EAX.
//   // blam-cc: EAX -> player_index, EBX -> forwarded_flag_object_index, EDI ->
//   //   forwarded_position
// UNSURE: forwarded_flag_object_index/forwarded_position identity (see
//   game_engine_ctf_reset_team_return_credit.c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "fn_game.h"
#include "fn_units.h"

extern data_array *player_data; // 0x0087a480
extern data_array *object_data; // 0x008603b0

extern void unit_dispatch_scripted_event_1b(uint8_t event_byte, uint32_t unit_index); // 0x56dcd0


// FIXED 2026-09-28: 0x4688f8 hands 0x468840 its own first stack argument (the flag object; 0x468a20 pushes it),
//   not forwarded registers.
// blam-cc: EAX -> player_index, stack -> flag_object_index
// Looks up the player's controlled unit; if that unit's object has network_role 0, dispatches
// scripted event 0x1b for it, then always drops its current weapon (forced) and resets its
// team's flag-return credit (game_engine_ctf_reset_team_return_credit.c, forwarding
// flag_object_index/position through, see UNSURE).
void game_engine_ctf_player_drop_flag(uint32_t player_index, datum_index flag_object_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    uint32_t unit_index = (uint32_t)p->unit;
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;

    if (unit_obj->network_role == 0) {
        unit_dispatch_scripted_event_1b(1, unit_index);
    }
    unit_drop_current_weapon(unit_index, 1);
    game_engine_ctf_reset_team_return_credit(flag_object_index);
}

#if 0
Original Ghidra decompilation (0x4688b0), from tools/pack.py 0x4688b0:

void FUN_004688b0(void)

{
  uint uVar1;
  uint in_EAX;

  uVar1 = *(uint *)((in_EAX & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
  if (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc) + 4) == 0) {
    FUN_0056dcd0(1);
  }
  unit_drop_current_weapon(uVar1,1);
  FUN_00468840();
  return;
}
#endif
