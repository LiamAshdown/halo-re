// hs_evaluate_unit_set_emotion  (not a Ghidra function; the evaluate handler of hs function 105 "unit_set_emotion" (unit, short -> void))
// address 0x47bd40, size 110 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47bd40, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47bd40..0x47bdae: a unit other than -1 gets its emotion byte (+0x2a8) from the short argument, then
//   object_copy_default_node_transforms(EAX unit, DX 6); returns 0.
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
extern void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); // 0x4f6b70, EAX, DX

void hs_evaluate_unit_set_emotion(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];

    if (unit != k_datum_index_none) {
        *(*(uint8_t **)((uint8_t *)object_data->data + (unit & 0xffff) * 0xc + 8) + 0x2a8) = *(uint8_t *)&arguments[1];
        object_copy_default_node_transforms(unit, 6);
    }
    hs_thread_return(0, thread_index);
    }
}
