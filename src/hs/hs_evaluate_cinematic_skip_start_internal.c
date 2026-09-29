// hs_evaluate_cinematic_skip_start_internal  (not a Ghidra function; the evaluate handler of hs function 298 "cinematic_skip_start_internal" (no parameters -> void))
// address 0x47f840, size 20 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f840, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f840..0x47f854: cinematic globals (0x006f187c) +0x0a = 1, returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "units.h"
#include "cutscene.h"
#include "fn_hs.h"


extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c

void hs_evaluate_cinematic_skip_start_internal(int16_t function_index, uint32_t thread_index, char first)
{
    cinematic_globals_ptr->skip_in_progress = 1;
    hs_thread_return(0, thread_index);
}
