// hs_evaluate_player_action_test_reset  (not a Ghidra function; the evaluate handler of hs function 349 "player_action_test_reset" (no parameters -> void))
// address 0x47f1f0, size 29 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f1f0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f1f0..0x47f20d: player control globals (*0x006b145c) +0x00 and +0x04 = 0, returns 0.
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

void hs_evaluate_player_action_test_reset(int16_t function_index, uint32_t thread_index, char first)
{
    *(uint32_t *)player_control_globals_ptr = 0;
    player_control_globals_ptr->action_flags_latched = 0;
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
