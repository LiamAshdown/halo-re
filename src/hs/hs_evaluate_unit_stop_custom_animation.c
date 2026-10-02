// hs_evaluate_unit_stop_custom_animation  (not a Ghidra function; the evaluate handler of hs function 99 "unit_stop_custom_animation" (unit -> void))
// address 0x47ba60, size 105 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47ba60, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47ba60..0x47bac9: a unit whose animation state byte (+0x2a3) is 0x1c goes back to state 0 (unit_try_set_animation_state, stack);
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
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern data_array *object_data; // 0x008603b0
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90

void hs_evaluate_unit_stop_custom_animation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];

    if (unit != k_datum_index_none &&
        *(*(uint8_t **)((uint8_t *)object_data->data + (unit & 0xffff) * 0xc + 8) + 0x2a3) == 0x1c) {
        unit_try_set_animation_state(unit, 0);
    }
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
