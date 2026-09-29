// hs_evaluate_debug_camera_load  (not a Ghidra function; the evaluate handler of hs function 253 "debug_camera_load" ( -> void))
// address 0x47f030, size 16 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47f030 trapped.
// WRITTEN 2026-09-28 from objdump 0x47f030..0x47f03f: calls camera_debug_load_from_file; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern void camera_debug_load_from_file(void); // 0x445940

void hs_evaluate_debug_camera_load(int16_t function_index, uint32_t thread_index, char first)
{
    camera_debug_load_from_file();
    hs_thread_return(0, thread_index);
}
