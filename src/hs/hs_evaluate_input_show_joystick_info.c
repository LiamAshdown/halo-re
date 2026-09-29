// hs_evaluate_input_show_joystick_info  (not a Ghidra function; the evaluate handler of hs function 450 "input_show_joystick_info" ( -> void))
// address 0x4819e0, size 16 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4819e0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4819e0..0x4819ef: calls input_device_list_print; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern void input_device_list_print(void); // 0x491750

void hs_evaluate_input_show_joystick_info(int16_t function_index, uint32_t thread_index, char first)
{
    input_device_list_print();
    hs_thread_return(0, thread_index);
}
