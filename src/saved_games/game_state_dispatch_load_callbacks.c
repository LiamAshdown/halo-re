// game_state_dispatch_load_callbacks  (Ghidra: game_state_dispatch_load_callbacks, already named)
// address 0x537f70, size 27 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase4/saved_games_functions.md; calls each of the 13 registered subsystem
// callback pointers at 0x0069e7b4 in sequence; out/phase4/saved_games_types_notes.md documents
// the table as game_state_after_load_procs[13].
// register convention: no parameters, no return value.

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
extern game_state_proc game_state_after_load_procs[k_game_state_after_load_proc_count]; // 0x0069e7b4

void game_state_dispatch_load_callbacks(void)
{
    game_state_proc *proc;
    int32_t count;

    proc = game_state_after_load_procs;
    count = k_game_state_after_load_proc_count;
    do {
        (*proc)();
        proc = proc + 1;
        count = count - 1;
    } while (count != 0);
}

#if 0
Original Ghidra decompilation (0x537f70):

void game_state_dispatch_load_callbacks(void)

{
  undefined **ppuVar1;
  int iVar2;

  ppuVar1 = &PTR_FUN_0069e7b4;
  iVar2 = 0xd;
  do {
    (*(code *)*ppuVar1)();
    ppuVar1 = ppuVar1 + 1;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
