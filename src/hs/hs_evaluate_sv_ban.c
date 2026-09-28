// hs_evaluate_sv_ban  (not a Ghidra function; the evaluate handler of hs function 497 "sv_ban" ( -> void))
// address 0x482c60, size 73 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482c60 trapped.
// WRITTEN 2026-09-28 from objdump 0x482c60..0x482ca8: evaluates the variadic arguments (0x48ad60) and, when done,
//   runs sv_ban (0x4e3990) with their count and values; returns 0 then.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern char hs_evaluate_variadic_arguments(uint32_t thread_index, int32_t value, uint32_t *out_count, int32_t **out_values); // 0x48ad60
extern void sv_ban(uint32_t argument_count, int32_t *arguments); // 0x4e3990, blam-cc: EAX, ECX

void hs_evaluate_sv_ban(int16_t function_index, uint32_t thread_index, char first)
{
    uint32_t count = 0;
    int32_t *values = 0;

    if (hs_evaluate_variadic_arguments(thread_index, (int32_t)first, &count, &values) != 0) {
        sv_ban(count, values);
        hs_thread_return(0, thread_index);
    }
}
