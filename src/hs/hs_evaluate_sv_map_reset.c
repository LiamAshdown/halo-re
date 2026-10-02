// hs_evaluate_sv_map_reset  (not a Ghidra function; the evaluate handler of hs function 491 "sv_map_reset" ( -> void))
// address 0x482ac0, size 16 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482ac0 trapped.
// WRITTEN 2026-09-28 from objdump 0x482ac0..0x482acf: calls sv_map_reset; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void sv_map_reset(void); // 0x4e2aa0

void hs_evaluate_sv_map_reset(int16_t function_index, uint32_t thread_index, char first)
{
    sv_map_reset();
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
