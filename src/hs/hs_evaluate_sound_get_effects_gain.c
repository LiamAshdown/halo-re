// hs_evaluate_sound_get_effects_gain  (not a Ghidra function; the evaluate handler of hs function 343 "sound_get_effects_gain" ( -> real))
// address 0x4801a0, size 21 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4801a0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4801a0..0x4801b4: returns sound_effects_gain (0x007252b0) as a real.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern float sound_effects_gain; // 0x007252b0

void hs_evaluate_sound_get_effects_gain(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return(*(int32_t *)&sound_effects_gain, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
