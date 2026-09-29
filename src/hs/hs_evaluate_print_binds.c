// hs_evaluate_print_binds  (not a Ghidra function; the evaluate handler of hs function 483 "print_binds" ( -> void))
// address 0x482480, size 16 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482480 trapped.
// WRITTEN 2026-09-28 from objdump 0x482480..0x48248f: calls input_print_bound_controls; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern void input_print_bound_controls(void); // 0x48bea0

void hs_evaluate_print_binds(int16_t function_index, uint32_t thread_index, char first)
{
    input_print_bound_controls();
    hs_thread_return(0, thread_index);
}
