// hs_evaluate_device_set_power  (not a Ghidra function; the evaluate handler of hs function 138 "device_set_power" (device, real -> void))
// address 0x47c930, size 135 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47c930, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47c930..0x47c9b7: a device other than -1 gets flags +0x1f4 |= 4 and power +0x1fc, then device_group_set_value(SI = its power
//   group +0x1f8, stack power); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_devices.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern data_array *object_data; // 0x008603b0


void hs_evaluate_device_set_power(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index device = (datum_index)arguments[0];
    float power = *(float *)&arguments[1];

    if (device != k_datum_index_none) {
        uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (device & 0xffff) * 0xc + 8);

        *(uint32_t *)(object + 0x1f4) |= 4;
        *(float *)(object + 0x1fc) = power;
        device_group_set_value(*(uint16_t *)(object + 0x1f8), power);
    }
    hs_thread_return(0, thread_index);
    }
}
