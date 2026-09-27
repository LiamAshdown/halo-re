// hs_evaluate_unit_has_weapon_readied  (not a Ghidra function; the evaluate handler of hs "unit_has_weapon_readied" (unit, object_definition -> boolean))
// address 0x47c5f0, size 87 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47c5f0, only reachable through that pointer.
//   Campaign track: 2 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47c5f0: ECX = the unit (+0x0), EDI = the weapon tag (+0x4); byte result zero-extended.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t unit_current_weapon_is_type(uint32_t unit_index, datum_index weapon_tag_id); // 0x561f80, ECX, EDI

void hs_evaluate_unit_has_weapon_readied(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t readied = unit_current_weapon_is_type((uint32_t)arguments[0], (datum_index)arguments[1]);
        hs_thread_return((int32_t)readied, thread_index);
    }
}
