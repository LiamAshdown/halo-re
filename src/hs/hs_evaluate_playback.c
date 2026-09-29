// hs_evaluate_playback  (not a Ghidra function; the evaluate handler of hs function 270 "playback" ( -> void))
// address 0x47f6c0, size 18 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47f6c0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47f6c0..0x47f6d1: sets 0x00719768 (playback requested); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern uint8_t playback_requested_00719768; // 0x00719768, UNSURE name

void hs_evaluate_playback(int16_t function_index, uint32_t thread_index, char first)
{
    playback_requested_00719768 = 1;
    hs_thread_return(0, thread_index);
}
