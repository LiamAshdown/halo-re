// hs_evaluate_sv_map_next  (not a Ghidra function; the evaluate handler of hs function 490 "sv_map_next" ( -> void))
// address 0x482aa0, size 26 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482aa0 trapped.
// WRITTEN 2026-09-28 from objdump 0x482aa0..0x482ab9: prints "sv_map_next is a dedicated server-only function!" (no
//   colour); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern void chimera__console_out(void *color, char *format, ...); // 0x496b50, blam-cc: EAX color

void hs_evaluate_sv_map_next(int16_t function_index, uint32_t thread_index, char first)
{
    chimera__console_out(0, "sv_map_next is a dedicated server-only function!"); // 0x0066dc7c
    hs_thread_return(0, thread_index);
}
