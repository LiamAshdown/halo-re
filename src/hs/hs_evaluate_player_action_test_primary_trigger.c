// hs_evaluate_player_action_test_primary_trigger  (not a Ghidra function; the evaluate handler of hs function 351 "player_action_test_primary_trigger" (no parameters -> boolean))
// address 0x47f240, size 39 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f240, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f240..0x47f267: bit 4 of the player control action flags (player control globals 0x006b145c +0x00) in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_hs.h"


extern player_control_globals *player_control_globals_ptr;

void hs_evaluate_player_action_test_primary_trigger(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 4) & 1), thread_index);
}
