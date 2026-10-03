// hs_evaluate_game_revert  (not a Ghidra function; the evaluate handler of hs function 316 "game_revert" (no parameters -> void))
// address 0x47fbd0, size 39 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fbd0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fbd0..0x47fbf7: main globals: bytes 0x0071973c and 0x0071974f = 0, word 0x00719754 = -1, byte 0x0071973a = 1; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t network_join_error_reason; // 0x0071973c
extern uint8_t main_globals_byte_0071974f; // 0x0071974f
extern uint16_t split_screen_quit_prompt_string; // 0x00719754
extern uint8_t main_globals_byte_0071973a; // 0x0071973a

void hs_evaluate_game_revert(int16_t function_index, uint32_t thread_index, char first)
{
    network_join_error_reason = 0;
    main_globals_byte_0071974f = 0;
    split_screen_quit_prompt_string = 0xffff;
    main_globals_byte_0071973a = 1;
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
