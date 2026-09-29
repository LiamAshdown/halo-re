// hs_evaluate_game_lost  (not a Ghidra function; the evaluate handler of hs function 306 "game_lost" ( -> void))
// address 0x47fa20, size 25 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47fa20 trapped.
// WRITTEN 2026-09-28 from objdump 0x47fa20..0x47fa38: clears 0x0071973c and sets 0x0071974f (game lost); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern uint8_t network_join_error_reason; // 0x0071973c
extern uint8_t main_globals_byte_0071974f; // 0x0071974f

void hs_evaluate_game_lost(int16_t function_index, uint32_t thread_index, char first)
{
    network_join_error_reason = 0;
    main_globals_byte_0071974f = 1;
    hs_thread_return(0, thread_index);
}
