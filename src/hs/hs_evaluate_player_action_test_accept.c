// hs_evaluate_player_action_test_accept  (not a Ghidra function; the evaluate handler of hs function 355 "player_action_test_accept" (no parameters -> boolean))
// address 0x47f310, size 59 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f310, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f310..0x47f34b: ORs 4 into player control globals (0x006b145c) +0x04 and +0x08, then returns bit 2 of the action flags (+0x00)
//   in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *player_control_globals_ptr; // 0x006b145c

void hs_evaluate_player_action_test_accept(int16_t function_index, uint32_t thread_index, char first)
{
    *(uint32_t *)(player_control_globals_ptr + 4) |= 4;
    *(uint32_t *)(player_control_globals_ptr + 8) |= 4;
    hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 2) & 1), thread_index);
}
