// hs_evaluate_device_group_get  (not a Ghidra function; the evaluate handler of hs "device_group_get" (device_group -> real))
// address 0x47cbe0, size 78 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47cbe0, only reachable through that pointer.
//   Campaign track: 20 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47cbe0: the group index (+0x0 word, zero-extended) reads device_groups record +0x4; the float is returned by its bits.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern data_array *device_groups; // 0x0087abf0, 8-byte records, the value (float) at +0x4

void hs_evaluate_device_group_get(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        float value = *(float *)((uint8_t *)device_groups->data + (uint16_t)*(uint16_t *)&arguments[0] * 8 + 4);
        hs_thread_return(*(int32_t *)&value, thread_index);
    }
}
