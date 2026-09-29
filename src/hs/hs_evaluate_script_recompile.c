// hs_evaluate_script_recompile  (not a Ghidra function; the evaluate handler of hs function 56 "script_recompile" ( -> void))
// address 0x47acd0, size 18 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47acd0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47acd0..0x47ace1: sets hs_reload_pending (0x006b14a8); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern uint8_t hs_reload_pending; // 0x006b14a8

void hs_evaluate_script_recompile(int16_t function_index, uint32_t thread_index, char first)
{
    hs_reload_pending = 1;
    hs_thread_return(0, thread_index);
}
