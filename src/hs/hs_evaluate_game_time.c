// hs_evaluate_game_time  (not a Ghidra function; the evaluate handler of hs "game_time" (-> long))
// address 0x47f080, size 17 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47f080, only reachable through that pointer.
//   Campaign track: 10 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47f080: returns the game tick (game time globals +0xc).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *game_time; // 0x006f1d6c

void hs_evaluate_game_time(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    
    hs_thread_return(*(int32_t *)(game_time + 0xc), thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
