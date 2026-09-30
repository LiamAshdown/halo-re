// hs_evaluate_breakable_surfaces_reset  (not a Ghidra function; the evaluate handler of hs "breakable_surfaces_reset" (-> void))
// address 0x47ce60, size 16 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47ce60, only reachable through that pointer.
//   Campaign track: 2 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47ce60: calls 0x4ffd40; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_objects.h"


void hs_evaluate_breakable_surfaces_reset(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    breakable_surfaces_reset();
    hs_thread_return(0, thread_index);
}
