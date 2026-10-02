// hs_evaluate_game_is_cooperative  (not a Ghidra function; the evaluate handler of hs function 310 "game_is_cooperative" (no parameters -> boolean))
// address 0x47faa0, size 37 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47faa0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47faa0..0x47fac5: the player count word (0x006894b8) above 1, in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern int16_t local_player_count; // 0x006894b8

void hs_evaluate_game_is_cooperative(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)(local_player_count > 1), thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
