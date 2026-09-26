// hs_evaluate_player0_joystick_set_is_normal  (not a Ghidra function; the evaluate handler of hs function 434 "player0_joystick_set_is_normal" (no parameters -> boolean))
// address 0x4814a0, size 41 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x4814a0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x4814a0..0x4814c9: 1 when profile byte 0x00712f05 is 0 or 1, in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t profile_globals_block[0x60a4]; // 0x00712dd8

void hs_evaluate_player0_joystick_set_is_normal(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t joystick_set = profile_globals_block[0x12d];

    hs_thread_return((int32_t)(joystick_set == 0 || joystick_set == 1), thread_index);
}
