// game_engine_ctf_respawn_team_flag  (Ghidra: FUN_00468430; named per its summary)
// address 0x468430, size 35 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: out/phase4/game_functions.md ("Respawns a team's objective (flag) object if that
//   team is configured to have one, recording the new object's handle"); disassembly
//   (objdump -d -M intel --start-address=0x468430 --stop-address=0x468460) shows the per-team
//   arrays at 0x006b0e88 (dword, indexed *4) and 0x006b0e90 (dword, indexed *4) and a team index
//   arriving live in ESI from the (unrecovered) caller; calls
//   game_engine_ctf_create_flag_object (0x468360, this batch), whose own two arguments (EAX
//   position pointer, stack name_index) are, like this function's own `team`, never assigned by
//   this function's body -- Ghidra shows the call with zero visible arguments -- so they must be
//   genuine pass-through register/stack values from this function's own (unrecovered) caller,
//   modeled the same way the already-committed game_engine_broadcast_kill_feed_to_team.c
//   (0x460ba0) models its own unrecovered forwarded arguments.
// register convention: team index in unaff_ESI; position/name_index forwarded straight through
//   to game_engine_ctf_create_flag_object without being read or written here.
//   // blam-cc: ESI -> team, EAX -> forwarded_position, stack -> forwarded_name_index
// UNSURE: 0x006b0e88's role is inferred from FUN_00468990 (this batch) treating the same array
//   as a per-team real_point3d* (the flag stand position); here it is only tested for non-NULL.
//   forwarded_position/forwarded_name_index names are guesses, matching the sibling precedent.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern real_point3d *ctf_team_flag_stand_position[2]; // 0x006b0e88, UNSURE exact element count
extern datum_index ctf_team_flag_object[2];           // 0x006b0e90

extern datum_index game_engine_ctf_create_flag_object(real_point3d *position, uint16_t name_index); // 0x468360, this batch

// blam-cc: ESI -> team, EAX -> forwarded_position, stack -> forwarded_name_index
// If `team` has a configured flag stand position, (re)creates its flag object (forwarding
// `forwarded_position`/`forwarded_name_index` straight through, see UNSURE above) and records
// the new handle.
void game_engine_ctf_respawn_team_flag(int32_t team, real_point3d *forwarded_position,
    uint16_t forwarded_name_index)
{
    if (ctf_team_flag_stand_position[team] != (real_point3d *)0) {
        datum_index new_flag = game_engine_ctf_create_flag_object(forwarded_position, forwarded_name_index);
        if (new_flag != (datum_index)0xffffffff) {
            ctf_team_flag_object[team] = new_flag;
        }
    }
}

#if 0
Original Ghidra decompilation (0x468430), from tools/pack.py 0x468430:

void FUN_00468430(void)

{
  int iVar1;
  int unaff_ESI;

  if ((&DAT_006b0e88)[unaff_ESI] != 0) {
    iVar1 = FUN_00468360();
    if (iVar1 != -1) {
      *(int *)(&DAT_006b0e90 + unaff_ESI * 4) = iVar1;
    }
  }
  return;
}
#endif
