// hs_evaluate_game_reverted  (not a Ghidra function; the evaluate handler of hs function 317 "game_reverted" (no parameters -> boolean))
// address 0x47fcb0, size 45 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fcb0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fcb0..0x47fcdd: game_state_revert_time (0x006e2ddc) == the game time (0x006f1d6c +0x0c), in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"
#include "fn_hs.h"


extern int32_t game_state_revert_time; // 0x006e2ddc
extern game_time_globals *game_time; // 0x006f1d6c

void hs_evaluate_game_reverted(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)(game_state_revert_time == game_time->game_time), thread_index);
}
