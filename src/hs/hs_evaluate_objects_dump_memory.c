// hs_evaluate_objects_dump_memory  (not a Ghidra function; the evaluate handler of hs function 73 "objects_dump_memory" ( -> void))
// address 0x47b230, size 16 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47b230 trapped.
// WRITTEN 2026-09-28 from objdump 0x47b230..0x47b23f: calls objects_dump_memory; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_objects.h"


void hs_evaluate_objects_dump_memory(int16_t function_index, uint32_t thread_index, char first)
{
    objects_dump_memory();
    hs_thread_return(0, thread_index);
}
