// hs_evaluate_garbage_collect_now  (not a Ghidra function; the evaluate handler of hs "garbage_collect_now" (-> void))
// address 0x47b410, size 20 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47b410, only reachable through that pointer.
//   Campaign track: 14 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47b410: sets byte +0x2 of the object globals (0x6b8cbc) to 1; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern uint8_t *object_globals_pointer; // 0x006b8cbc

void hs_evaluate_garbage_collect_now(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    object_globals_pointer[2] = 1; // requests a collection on the next update
    hs_thread_return(0, thread_index);
}
