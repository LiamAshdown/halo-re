// hs_evaluate_rcon  (not a Ghidra function; the evaluate handler of hs function 493 "rcon" ( -> void))
// address 0x4829c0, size 78 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4829c0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4829c0..0x482a0d: evaluates the variadic string arguments (0x48ad60) and, when
//   done, runs rcon (0x4e4c00) with their count and values; returns 0 then.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern char hs_evaluate_variadic_arguments(uint32_t thread_index, int32_t value, uint32_t *out_count, int32_t **out_values); // 0x48ad60
extern void rcon(int32_t argument_count, char **arguments); // 0x4e4c00

void hs_evaluate_rcon(int16_t function_index, uint32_t thread_index, char first)
{
    uint32_t count = 0;
    int32_t *values = 0;

    if (hs_evaluate_variadic_arguments(thread_index, (int32_t)first, &count, &values) != 0) {
        rcon((int32_t)count, (char **)values);
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
