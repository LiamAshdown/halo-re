// game_state_after_load_restore_time  (Ghidra: not exported as a standalone function, named)
// address 0x5385d0, size 31 bytes (0x5385d0..0x5385ec, one ret at 0x5385ed)
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: out/phase4/saved_games_types_notes.md "In-range functions missing from the
// 116-function list": "0x5385d0 (after-load proc 10, stores the game tick in 0x006e2ddc)";
// this address is not present in out/functions.json, so there is no Ghidra decompilation to
// pack -- rewritten directly from objdump (a short, straight-line function with no branches
// worth double-checking). It is entry 10 of game_state_after_load_procs (0x0069e7b4), called
// by game_state_dispatch_load_callbacks after every load/revert.
// UNSURE: this reads and writes game_time_globals::unknown_00 / ::active / ::paused, though
// types/game.h's comment on unknown_00 says "never read or written" elsewhere in the image --
// this function is the one place that does touch it; left as-is since types/game.h cannot be
// edited here.
// register convention: no parameters, no return value.
// reconciled: R33 game_time_globals.unknown_00 -> initialized (uint8 at +0x00, same byte)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t game_state_revert_time; // 0x006e2ddc

void game_state_after_load_restore_time(void)
{
    game_state_revert_time = game_time->game_time;
    game_time->paused = 0;
    if (game_time->initialized != 0) {
        game_time->active = 1;
    }
}

#if 0
Original disassembly (0x5385d0, no Ghidra export -- not in out/functions.json):

005385d0:
  a1 6c 1d 6f 00       mov    eax,ds:0x6f1d6c
  8b 48 0c             mov    ecx,[eax+0xc]
  89 0d dc 2d 6e 00     mov    ds:0x6e2ddc,ecx
  8a 10                mov    dl,[eax]
  32 c9                xor    cl,cl
  3a d1                cmp    dl,cl
  88 48 02             mov    [eax+0x2],cl
  74 04                je     0x5385ed
  c6 40 01 01          mov    byte ptr [eax+0x1],0x1
005385ed:
  c3                   ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
