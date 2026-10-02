// hs_evaluate_unit_set_emotion_animation  (not a Ghidra function; the evaluate handler of hs function 110 "unit_set_emotion_animation" (unit, string -> void))
// address 0x47bf50, size 66 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47bf50 trapped.
// WRITTEN 2026-09-28 from objdump 0x47bf50..0x47bf91: unit_scripting_set_emotion_animation (0x569cf0) with the unit
//   and the name; returns 0.
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
extern void unit_scripting_set_emotion_animation(uint32_t unit_index, const char *emotion_name); // 0x569cf0, blam-cc: EAX, ECX

void hs_evaluate_unit_set_emotion_animation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        unit_scripting_set_emotion_animation((uint32_t)arguments[0], (const char *)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
