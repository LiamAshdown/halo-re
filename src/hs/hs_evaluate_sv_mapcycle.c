// hs_evaluate_sv_mapcycle  (not a Ghidra function; the evaluate handler of hs function 486 "sv_mapcycle" ( -> void))
// address 0x482e10, size 26 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482e10 trapped.
// WRITTEN 2026-09-28 from objdump 0x482e10..0x482e29: prints "sv_mapcycle is a dedicated server-only function!" (no
//   colour); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void chimera__console_out(void *color, char *format, ...); // 0x496b50, blam-cc: EAX color

void hs_evaluate_sv_mapcycle(int16_t function_index, uint32_t thread_index, char first)
{
    chimera__console_out(0, (char *)"sv_mapcycle is a dedicated server-only function!"); // 0x0066dd20
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
