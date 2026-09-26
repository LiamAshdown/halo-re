// hs_evaluate_game_saving  (not a Ghidra function; the evaluate handler of hs function 315 "game_saving" (no parameters -> boolean))
// address 0x47fbb0, size 31 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fbb0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fbb0..0x47fbcf: main globals byte 0x0071973c in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t main_globals_byte_0071973c; // 0x0071973c

void hs_evaluate_game_saving(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)main_globals_byte_0071973c, thread_index);
}
