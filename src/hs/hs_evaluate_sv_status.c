// hs_evaluate_sv_status  (not a Ghidra function; the evaluate handler of hs function 502 "sv_status" ( -> void))
// address 0x482c50, size 16 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482c50 trapped.
// WRITTEN 2026-09-28 from objdump 0x482c50..0x482c5f: calls sv_status; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_networking.h"


void hs_evaluate_sv_status(int16_t function_index, uint32_t thread_index, char first)
{
    sv_status();
    hs_thread_return(0, thread_index);
}
