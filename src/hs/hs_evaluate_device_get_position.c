// hs_evaluate_device_get_position  (not a Ghidra function; the evaluate handler of hs function 140 "device_get_position" (device -> real))
// address 0x47cae0, size 114 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47cae0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47cae0..0x47cb52: the device's position (+0x208), 0 for -1.
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

void hs_evaluate_device_get_position(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index device = (datum_index)arguments[0];
    int32_t result = 0;

    if (device != k_datum_index_none) {
        result = *(int32_t *)(*(uint8_t **)((uint8_t *)object_data->data + (device & 0xffff) * 0xc + 8) + 0x208);
    }
    hs_thread_return(result, thread_index);
    }
}
