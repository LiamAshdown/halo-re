// game_engine_get_time_remaining  (Ghidra: FUN_0045cab0; named per
// out/phase4/game_functions.md, "Returns the number of ticks remaining before the game's time
// limit expires, or -1 if there is no time limit.")
// address 0x45cab0, size 37 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/game.h game_variant::unknown_78 (live copy at 0x006f1d00 == 0x006f1c88+0x78;
//   the header calls it unknown but notes "slayer default 36000 ticks (20 minutes)", which is
//   exactly a time limit field -- used here divided against the running clock, corroborating
//   that reading); game_time_globals::game_time (0x006f1d6c+0x0c); global 0x0087aa20
//   game_engine_unknown_aa20.
// register convention: __cdecl, no arguments.
//
// UNSURE: types/game.h leaves 0x006f1d00's field (game_variant + 0x78) named unknown_78; this
// function's own arithmetic (limit minus elapsed, adjusted by an unknown correction term) is
// strong evidence it is the match/round time limit in ticks, but the header is not renamed
// here since only one function's evidence is not enough to retire "unknown". 0x0087aa20's role
// is not established beyond "some additive correction to the deadline".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_variant game_engine_variant; // 0x006f1c88 (::unknown_78 at 0x006f1d00, time limit)
extern game_time_globals *game_time;      // 0x006f1d6c
extern int32_t game_engine_round_reset_tick;          // 0x0087aa20

// Returns -1 if the variant has no time limit; otherwise the ticks remaining (never negative).
int32_t game_engine_get_time_remaining(void)
{
    int32_t remaining;

    remaining = -1;
    if (0 < game_engine_variant.unknown_78) {
        remaining = (game_engine_variant.unknown_78 - game_time->game_time) + game_engine_round_reset_tick;
        if (remaining < 0) {
            remaining = 0;
        }
    }
    return remaining;
}

#if 0
Original Ghidra decompilation (0x45cab0), from tools/pack.py 0x45cab0:

int FUN_0045cab0(void)

{
  int iVar1;

  iVar1 = -1;
  if ((0 < DAT_006f1d00) &&
     (iVar1 = (DAT_006f1d00 - *(int *)(DAT_006f1d6c + 0xc)) + DAT_0087aa20, iVar1 < 0)) {
    iVar1 = 0;
  }
  return iVar1;
}
#endif
