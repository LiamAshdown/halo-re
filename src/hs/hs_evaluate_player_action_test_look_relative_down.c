// hs_evaluate_player_action_test_look_relative_down  (not a Ghidra function; the evaluate handler of hs function 358 "player_action_test_look_relative_down" (no parameters -> boolean))
// address 0x47f3c0, size 39 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f3c0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f3c0..0x47f3e7: bit 8 of the player control action flags (player control globals 0x006b145c +0x00) in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *player_control_globals_ptr; // 0x006b145c

void hs_evaluate_player_action_test_look_relative_down(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 8) & 1), thread_index);
}
