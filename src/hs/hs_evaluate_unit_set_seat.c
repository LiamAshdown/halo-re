// hs_evaluate_unit_set_seat  (not a Ghidra function; the evaluate handler of hs function 119 "unit_set_seat" (unit, string -> void))
// address 0x47c260, size 102 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47c260, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47c260..0x47c2c6: a unit other than -1 gets byte +0x20f = unit_base_animation_state_from_name(EDI name); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern data_array *object_data; // 0x008603b0
extern int16_t unit_base_animation_state_from_name(const char *name); // 0x56eb90, blam-cc: EDI

void hs_evaluate_unit_set_seat(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];

    if (unit != k_datum_index_none) {
        uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (unit & 0xffff) * 0xc + 8);

        object[0x20f] = (uint8_t)unit_base_animation_state_from_name((const char *)arguments[1]);
    }
    hs_thread_return(0, thread_index);
    }
}
