// hs_evaluate_player_action_test_back  (not a Ghidra function; the evaluate handler of hs function 356 "player_action_test_back" (no parameters -> boolean))
// address 0x47f350, size 59 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f350, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f350..0x47f38b: ORs 8 into player control globals (0x006b145c) +0x04 and +0x08, then returns bit 3 of the action flags (+0x00)
//   in a zeroed dword.
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

void hs_evaluate_player_action_test_back(int16_t function_index, uint32_t thread_index, char first)
{
    player_control_globals_ptr->action_flags_latched |= 8;
    player_control_globals_ptr->action_flags_edge |= 8;
    hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 3) & 1), thread_index);
}
