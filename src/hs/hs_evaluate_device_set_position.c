// hs_evaluate_device_set_position  (not a Ghidra function; the evaluate handler of hs function 139 "device_set_position" (device, real -> boolean))
// address 0x47ca40, size 160 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47ca40, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47ca40..0x47cae0: device_group_set_value(SI = the device's position group +0x204, stack position) into a zeroed dword; 0 for a
//   -1 device or a -1 group.
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
extern uint8_t device_group_set_value(uint16_t group_index, float value); // 0x44bd70, blam-cc: ESI, stack

void hs_evaluate_device_set_position(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index device = (datum_index)arguments[0];
    uint8_t result = 0;

    if (device != k_datum_index_none) {
        uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (device & 0xffff) * 0xc + 8);
        uint16_t group = *(uint16_t *)(object + 0x204);

        if (group != 0xffff) {
            result = device_group_set_value(group, *(float *)&arguments[1]);
        }
    }
    hs_thread_return((int32_t)result, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
