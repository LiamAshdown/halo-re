// hs_evaluate_numeric_countdown_timer_stop  (not a Ghidra function; the evaluate handler of hs "numeric_countdown_timer_stop" (-> void))
// address 0x47aec0, size 18 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47aec0, only reachable through that pointer.
//   Campaign track: 1 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47aec0: clears the running flag at 0x721e54; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t numeric_countdown_timer_running; // 0x00721e54

void hs_evaluate_numeric_countdown_timer_stop(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    numeric_countdown_timer_running = 0;
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
