// hs_evaluate_sv_players  (not a Ghidra function; the evaluate handler of hs function 495 "sv_players" ( -> void))
// address 0x482c00, size 16 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482c00 trapped.
// WRITTEN 2026-09-28 from objdump 0x482c00..0x482c0f: calls sv_players; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void sv_players(void); // 0x4e2c70

void hs_evaluate_sv_players(int16_t function_index, uint32_t thread_index, char first)
{
    sv_players();
    hs_thread_return(0, thread_index);
}
