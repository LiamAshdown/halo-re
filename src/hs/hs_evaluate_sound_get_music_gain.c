// hs_evaluate_sound_get_music_gain  (not a Ghidra function; the evaluate handler of hs function 341 "sound_get_music_gain" ( -> real))
// address 0x480130, size 21 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_480130 trapped.
// WRITTEN 2026-09-28 from objdump 0x480130..0x480144: returns sound_music_gain (0x007252a8) as a real.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern float sound_music_gain; // 0x007252a8

void hs_evaluate_sound_get_music_gain(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return(*(int32_t *)&sound_music_gain, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
