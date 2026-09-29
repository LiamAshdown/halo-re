// hs_evaluate_game_won  (not a Ghidra function; the evaluate handler of hs function 305 "game_won" (no parameters -> void))
// address 0x47fa00, size 25 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fa00, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fa00..0x47fa19: main globals: byte 0x0071973c = 0, byte 0x0071974e = 1; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern uint8_t network_join_error_reason; // 0x0071973c
extern uint8_t main_globals_byte_0071974e; // 0x0071974e

void hs_evaluate_game_won(int16_t function_index, uint32_t thread_index, char first)
{
    network_join_error_reason = 0;
    main_globals_byte_0071974e = 1;
    hs_thread_return(0, thread_index);
}
