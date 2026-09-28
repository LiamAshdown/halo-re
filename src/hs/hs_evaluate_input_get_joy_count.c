// hs_evaluate_input_get_joy_count  (not a Ghidra function; the evaluate handler of hs function 445 "input_get_joy_count" (no parameters -> short))
// address 0x4817f0, size 33 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x4817f0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x4817f0..0x481811: the joystick count (0x006b1844) as a word in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern int32_t input_device_count; // 0x006b1844

void hs_evaluate_input_get_joy_count(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)(uint16_t)input_device_count, thread_index);
}
