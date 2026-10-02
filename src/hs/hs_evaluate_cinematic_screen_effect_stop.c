// hs_evaluate_cinematic_screen_effect_stop  (not a Ghidra function; the evaluate handler of hs function 427 "cinematic_screen_effect_stop" (no parameters -> void))
// address 0x481340, size 24 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x481340, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x481340..0x481358: the screen effect state (*0x0071cfc4), when present, gets +0x38 = 0; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *cinematic_screen_effect_state; // 0x0071cfc4

void hs_evaluate_cinematic_screen_effect_stop(int16_t function_index, uint32_t thread_index, char first)
{
    if (cinematic_screen_effect_state != 0) {
        cinematic_screen_effect_state[0x38] = 0;
    }
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
