// hs_evaluate_script_doc  (not a Ghidra function; the evaluate handler of hs function 57 "script_doc" ( -> void))
// address 0x47acf0, size 16 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47acf0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47acf0..0x47acff: calls hs_doc; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void hs_doc(void); // 0x484270

void hs_evaluate_script_doc(int16_t function_index, uint32_t thread_index, char first)
{
    hs_doc();
    hs_thread_return(0, thread_index);
}
