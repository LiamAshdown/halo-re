// hs_evaluate_player_action_test_move_relative_all_directions  (not a Ghidra function; the evaluate handler of hs function 362 "player_action_test_move_relative_all_directions" (no parameters -> boolean))
// address 0x47f480, size 47 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f480, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f480..0x47f4af: 1 when every bit of 0x7800 is set in the player control action flags (0x006b145c +0x00), in a zeroed dword.
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

void hs_evaluate_player_action_test_move_relative_all_directions(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)((~*(uint32_t *)player_control_globals_ptr & 0x7800) == 0), thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
