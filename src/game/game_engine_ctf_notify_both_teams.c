// game_engine_ctf_notify_both_teams  (Ghidra: FUN_00468460; named per its summary)
// address 0x468460, size 64 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Invokes a per-team update/notify routine for both
//   the current team and its opposing team"); calls game_engine_broadcast_kill_feed_to_team
//   (0x460ba0). The `(x & 0x80000001)` / two's-complement fixup pair is signed modulo 2, i.e. C `% 2`.
// register convention: team in EAX only; the calls load ESI (0x2f / 0x2e) and BL (1) themselves.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void game_engine_broadcast_kill_feed_to_team(int32_t message_type, int32_t team, uint8_t broadcast); // 0x460ba0, blam-cc: ESI message_type, BL broadcast, stack team

// FIXED 2026-09-28 from objdump 0x468460..0x46849f: only EAX (the team) is an input; the calls load ESI = 0x2f / 0x2e
//   and BL = 1 themselves.
// blam-cc: EAX -> team
void game_engine_ctf_notify_both_teams(int32_t team)
{
    game_engine_broadcast_kill_feed_to_team(0x2f, team % 2, 1);
    game_engine_broadcast_kill_feed_to_team(0x2e, (team + 1) % 2, 1);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
