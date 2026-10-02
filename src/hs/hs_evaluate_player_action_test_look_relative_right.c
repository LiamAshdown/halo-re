// hs_evaluate_player_action_test_look_relative_right  (not a Ghidra function; the evaluate handler of hs function 360 "player_action_test_look_relative_right" (no parameters -> boolean))
// address 0x47f420, size 39 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f420, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f420..0x47f447: bit 10 of the player control action flags (player control globals 0x006b145c +0x00) in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "units.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern player_control_globals *player_control_globals_ptr;

void hs_evaluate_player_action_test_look_relative_right(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 10) & 1), thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
