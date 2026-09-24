// game_engine_ctf_notify_both_teams  (Ghidra: FUN_00468460; named per its summary)
// address 0x468460, size 64 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Invokes a per-team update/notify routine for both
//   the current team and its opposing team"); calls game_engine_broadcast_kill_feed_to_team
//   (0x460ba0, already committed) which itself needs four more forwarded values (broadcast
//   gate in ESI, message_type/subject/broadcast flag) that this function's own body never
//   touches either -- so, like that function's own header documents, they must flow straight
//   through from this function's caller. The `(x & 0x80000001)` / two's-complement fixup pair
//   is the same signed-modulo-2 idiom already documented literally (not simplified to `% 2`) in
//   game_engine_resolve_player_team.c.
// register convention: team in_EAX; broadcast_enabled/message_type/subject/broadcast are
//   forwarded straight through to both calls without being read or written here.
//   // blam-cc: EAX -> team, ESI -> forwarded_broadcast_enabled, stack ->
//   //   forwarded_message_type, forwarded_subject, forwarded_broadcast
// UNSURE: the forwarded_* parameter names/positions are guesses, matching the sibling
//   game_engine_broadcast_kill_feed_to_team.c precedent.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern void game_engine_broadcast_kill_feed_to_team(int32_t broadcast_enabled, int32_t team,
    uint32_t forwarded_message_type, datum_index forwarded_subject, char forwarded_broadcast); // 0x460ba0

// blam-cc: EAX -> team, ESI -> forwarded_broadcast_enabled, stack -> forwarded_message_type,
//   forwarded_subject, forwarded_broadcast
// Notifies both `team` and its opposite (team XOR 1, computed via signed modulo-2 so that -1
// maps to -1 rather than 1) through game_engine_broadcast_kill_feed_to_team.
void game_engine_ctf_notify_both_teams(int32_t team, int32_t forwarded_broadcast_enabled,
    uint32_t forwarded_message_type, datum_index forwarded_subject, char forwarded_broadcast)
{
    uint32_t team_a = (uint32_t)team & 0x80000001;
    if ((int32_t)team_a < 0) {
        team_a = (team_a - 1 | 0xfffffffe) + 1;
    }
    game_engine_broadcast_kill_feed_to_team(forwarded_broadcast_enabled, (int32_t)team_a,
        forwarded_message_type, forwarded_subject, forwarded_broadcast);

    {
        uint32_t team_b = ((uint32_t)team + 1) & 0x80000001;
        if ((int32_t)team_b < 0) {
            team_b = (team_b - 1 | 0xfffffffe) + 1;
        }
        game_engine_broadcast_kill_feed_to_team(forwarded_broadcast_enabled, (int32_t)team_b,
            forwarded_message_type, forwarded_subject, forwarded_broadcast);
    }
}

#if 0
Original Ghidra decompilation (0x468460), from tools/pack.py 0x468460:

void FUN_00468460(void)

{
  uint in_EAX;
  uint uVar1;

  uVar1 = in_EAX & 0x80000001;
  if ((int)uVar1 < 0) {
    uVar1 = (uVar1 - 1 | 0xfffffffe) + 1;
  }
  FUN_00460ba0(uVar1);
  uVar1 = in_EAX + 1 & 0x80000001;
  if ((int)uVar1 < 0) {
    uVar1 = (uVar1 - 1 | 0xfffffffe) + 1;
  }
  FUN_00460ba0(uVar1);
  return;
}
#endif
