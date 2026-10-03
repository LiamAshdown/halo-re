// hs_evaluate_player_action_test_jump  (not a Ghidra function; the evaluate handler of hs "player_action_test_jump" (-> boolean))
// address 0x47f210, size 38 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47f210, only reachable through that pointer.
//   Campaign track: 1 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47f210: bit 0x2 (jump) of the action test results (control globals +0x0), zero-extended.
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

void hs_evaluate_player_action_test_jump(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    uint8_t jumped = (uint8_t)((*(uint32_t *)player_control_globals_ptr >> 1) & 1);
    hs_thread_return((int32_t)jumped, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
