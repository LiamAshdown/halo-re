// hs_evaluate_player_action_test_action  (not a Ghidra function; the evaluate handler of hs function 354 "player_action_test_action" (no parameters -> boolean))
// address 0x47f2d0, size 51 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f2d0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f2d0..0x47f303: ORs 1 into player control globals (0x006b145c) +0x04 and +0x08, then returns bit 0 of the action flags (+0x00)
//   in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *player_control_globals_ptr; // 0x006b145c

void hs_evaluate_player_action_test_action(int16_t function_index, uint32_t thread_index, char first)
{
    *(uint32_t *)(player_control_globals_ptr + 4) |= 1;
    *(uint32_t *)(player_control_globals_ptr + 8) |= 1;
    hs_thread_return((int32_t)(player_control_globals_ptr[0] & 1), thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
