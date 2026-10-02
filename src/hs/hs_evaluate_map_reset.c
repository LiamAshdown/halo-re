// hs_evaluate_map_reset  (not a Ghidra function; the evaluate handler of hs function 262 "map_reset" ( -> void))
// address 0x47f500, size 39 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47f500 trapped.
// WRITTEN 2026-09-28 from objdump 0x47f500..0x47f526: clears 0x0071973c and 0x0071974f, resets the quit prompt
//   string to -1 and sets 0x00719738 (the map reset request); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t network_join_error_reason; // 0x0071973c
extern uint8_t main_globals_byte_0071974f; // 0x0071974f
extern uint16_t split_screen_quit_prompt_string; // 0x00719754
extern uint8_t unknown_00719738; // 0x00719738, UNSURE

void hs_evaluate_map_reset(int16_t function_index, uint32_t thread_index, char first)
{
    network_join_error_reason = 0;
    main_globals_byte_0071974f = 0;
    split_screen_quit_prompt_string = 0xffff;
    unknown_00719738 = 1;
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
