// hs_evaluate_ai_detach_units  (not a Ghidra function; the evaluate handler of the orphan hs function record 0x658a40 "ai_detach_units" (object_list -> void), not in hs_function_definitions)
// address 0x47d1b0, size 63 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47d1b0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47d1b0..0x47d1ee: clears the orders of every unit in the list (0x432ad0);
//   returns 0.
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
extern void ai_object_list_clear_orders_with_weapon(datum_index object_list_header_handle); // 0x432ad0, blam-cc: ECX

void hs_evaluate_ai_detach_units(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_object_list_clear_orders_with_weapon((datum_index)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
