// hs_evaluate_numeric_countdown_timer_restart  (not a Ghidra function; the evaluate handler of hs function 64 "numeric_countdown_timer_restart" ( -> void))
// address 0x47aee0, size 18 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47aee0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47aee0..0x47aef1: sets numeric_countdown_timer_running (0x00721e54); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t numeric_countdown_timer_running; // 0x00721e54

void hs_evaluate_numeric_countdown_timer_restart(int16_t function_index, uint32_t thread_index, char first)
{
    numeric_countdown_timer_running = 1;
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
