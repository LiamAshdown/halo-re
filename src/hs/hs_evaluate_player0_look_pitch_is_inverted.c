// hs_evaluate_player0_look_pitch_is_inverted  (not a Ghidra function; the evaluate handler of hs function 433 "player0_look_pitch_is_inverted" (no parameters -> boolean))
// address 0x481480, size 31 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x481480, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x481480..0x48149f: profile byte 0x00712f07 in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t profile_globals_block[0x60a4]; // 0x00712dd8

void hs_evaluate_player0_look_pitch_is_inverted(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)profile_globals_block[0x12f], thread_index);
}
