// hs_evaluate_sv_mapcycle_add  (not a Ghidra function; the evaluate handler of hs function 488 "sv_mapcycle_add" (string, string -> void))
// address 0x482e30, size 71 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482e30 trapped.
// WRITTEN 2026-09-28 from objdump 0x482e30..0x482e76: prints "sv_mapcycle_add is a dedicated server-only function!"
//   (no colour); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void chimera__console_out(void *color, char *format, ...); // 0x496b50, blam-cc: EAX color

void hs_evaluate_sv_mapcycle_add(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        chimera__console_out(0, (char *)"sv_mapcycle_add is a dedicated server-only function!"); // 0x0066dce8
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
