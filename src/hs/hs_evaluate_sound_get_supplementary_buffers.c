// hs_evaluate_sound_get_supplementary_buffers  (not a Ghidra function; the evaluate handler of hs function 442 "sound_get_supplementary_buffers" ( -> short))
// address 0x481720, size 33 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_481720 trapped.
// WRITTEN 2026-09-28 from objdump 0x481720..0x481740: returns the supplementary buffer count (0x00746122) as a
//   short.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern int16_t sound_supplementary_buffers_00746122; // 0x00746122, UNSURE name

void hs_evaluate_sound_get_supplementary_buffers(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)(uint16_t)(sound_supplementary_buffers_00746122), thread_index);
}
