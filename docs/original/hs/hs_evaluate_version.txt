// hs_evaluate_version  (not a Ghidra function; the evaluate handler of hs function 269 "version" ( -> void))
// address 0x47f6a0, size 26 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47f6a0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47f6a0..0x47f6b9: prints the build string to the console
//   (console_print_error_va, not cleared first); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, blam-cc: AL

void hs_evaluate_version(int16_t function_index, uint32_t thread_index, char first)
{
    console_print_error_va(0, "halo pc 01.00.10.0621 Apr 16 2014 15:54:48"); // 0x0066b25c
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
