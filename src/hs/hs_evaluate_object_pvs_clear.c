// hs_evaluate_object_pvs_clear  (not a Ghidra function; the evaluate handler of hs function 87 "object_pvs_clear" (no parameters -> void))
// address 0x47b680, size 25 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b680, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b680..0x47b699: object_globals (0x006b8cbc) +0x90 word = 0, returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "fn_hs.h"


extern object_globals *object_globals_pointer;

void hs_evaluate_object_pvs_clear(int16_t function_index, uint32_t thread_index, char first)
{
    object_globals_pointer->ambient_cluster_mode = 0;
    hs_thread_return(0, thread_index);
}
