// hs_evaluate_sv_banlist  (not a Ghidra function; the evaluate handler of hs function 498 "sv_banlist" ( -> void))
// address 0x482a10, size 16 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482a10 trapped.
// WRITTEN 2026-09-28 from objdump 0x482a10..0x482a1f: calls network_banlist_print; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern void network_banlist_print(void); // 0x4e34e0

void hs_evaluate_sv_banlist(int16_t function_index, uint32_t thread_index, char first)
{
    network_banlist_print();
    hs_thread_return(0, thread_index);
}
