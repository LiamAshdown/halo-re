// hs_evaluate_quit  (not a Ghidra function; the evaluate handler of hs function 271 "quit" ( -> void))
// address 0x482820, size 33 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482820 trapped.
// WRITTEN 2026-09-28 from objdump 0x482820..0x482840: quit: sets 0x0071975b, movie_playback_abort (0x007196d4) = 1
//   and disarms the split screen quit prompt; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern uint8_t ui_event_byte_0071975b; // 0x0071975b
extern int32_t movie_playback_abort; // 0x007196d4
extern uint8_t split_screen_quit_prompt_armed; // 0x00719757

void hs_evaluate_quit(int16_t function_index, uint32_t thread_index, char first)
{
    ui_event_byte_0071975b = 1;
    movie_playback_abort = 1;
    split_screen_quit_prompt_armed = 0;
    hs_thread_return(0, thread_index);
}
