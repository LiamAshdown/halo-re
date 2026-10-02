// hs_evaluate_unit_has_weapon  (not a Ghidra function; the evaluate handler of hs function 127 "unit_has_weapon" (unit, object_definition -> boolean))
// address 0x47c580, size 101 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47c580 trapped.
// WRITTEN 2026-09-28 from objdump 0x47c580..0x47c5e4: true when both the unit and the weapon definition are given
//   and the unit carries that weapon (0x56d610).
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
extern uint8_t unit_has_weapon_of_type(uint32_t unit_index, int32_t weapon_group_tag); // 0x56d610, blam-cc: EAX unit, EBX tag

void hs_evaluate_unit_has_weapon(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t has = 0;

        if (arguments[0] != -1 && arguments[1] != -1) {
            has = unit_has_weapon_of_type((uint32_t)arguments[0], arguments[1]);
        }
        hs_thread_return((int32_t)(uint8_t)(has), thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
