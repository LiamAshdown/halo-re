// game_state_perform_save  (Ghidra: FUN_005381c0, renamed)
// address 0x5381c0, size 50 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/saved_games_functions.md summary "Top-level entry point that triggers a
// saved-game snapshot/commit: runs a pre-save callback, queues the async write, and records
// whether it succeeded." Named to parallel the existing game_state_perform_revert (0x538200).
// Phase 4 review: matched objdump 0x5381c0..0x5381f1; game_state_queue_write returns a bool in AL
// only, now declared uint8_t.
// register convention: is_checkpoint is the recognized stack parameter (param_1), matching
// game_state_queue_write's own parameter, which it forwards unchanged.
// UNSURE: 0x00719769 / 0x0071976a are outside this module's documented globals (not listed in
// types/saved_games.h); treated as UI busy/done flags the interface polls, left as opaque
// externs.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern game_state_proc game_state_before_save_proc; // 0x0069e7ac
extern uint8_t unknown_00719769; // 0x00719769, UNSURE: interface busy flag
extern uint8_t unknown_0071976a; // 0x0071976a, UNSURE: interface done flag
extern uint8_t game_state_revert_available; // 0x006e2dd9

extern uint8_t game_state_queue_write(uint8_t is_checkpoint); // 0x538700, returns in AL

void game_state_perform_save(uint8_t is_checkpoint)
{
    uint8_t result;

    game_state_before_save_proc();
    unknown_00719769 = 0;
    unknown_0071976a = 0;
    result = game_state_queue_write(is_checkpoint);
    game_state_revert_available = result != 0;
    unknown_0071976a = 1;
}

#if 0
Original Ghidra decompilation (0x5381c0):

void FUN_005381c0(char param_1)

{
  int iVar1;

  (*(code *)PTR_FUN_0069e7ac)();
  DAT_00719769 = 0;
  DAT_0071976a = 0;
  iVar1 = game_state_queue_write(param_1);
  DAT_006e2dd9 = (char)iVar1 != '\0';
  DAT_0071976a = 1;
  return;
}
#endif
