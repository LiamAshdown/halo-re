// hs_evaluate_units_set_maximum_vitality  (not a Ghidra function; the evaluate handler of hs function 113 "units_set_maximum_vitality" (object_list, real, real -> void))
// address 0x47c060, size 74 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47c060 trapped.
// WRITTEN 2026-09-28 from objdump 0x47c060..0x47c0a9: sets the maximum vitalities of every object in the list
//   (0x561ab0; body and shield reals by value); returns 0.
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
extern void ai_object_list_initialize_shield_stun_thresholds(datum_index object_list_header_handle, float override_max_body_vitality,
    float override_max_shield_vitality); // 0x561ab0, blam-cc: ECX list, stack body, shield

void hs_evaluate_units_set_maximum_vitality(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_object_list_initialize_shield_stun_thresholds((datum_index)arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
