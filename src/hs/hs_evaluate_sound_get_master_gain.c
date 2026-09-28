// hs_evaluate_sound_get_master_gain  (not a Ghidra function; the evaluate handler of hs function 339 "sound_get_master_gain" ( -> real))
// address 0x4800c0, size 21 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4800c0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4800c0..0x4800d4: returns sound_master_gain (0x007252ac) as a real.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern float sound_master_gain; // 0x007252ac

void hs_evaluate_sound_get_master_gain(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return(*(int32_t *)&sound_master_gain, thread_index);
}
