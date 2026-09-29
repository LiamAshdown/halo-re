// hs_evaluate_device_group_set_immediate  (not a Ghidra function; the evaluate handler of hs "device_group_set_immediate" (device_group, real -> void))
// address 0x47cc90, size 73 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47cc90, only reachable through that pointer.
//   Campaign track: 11 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47cc90: SI = the group (+0x0), stack the value (+0x4); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void device_group_set_value_immediate(uint16_t group_index, float value); // 0x44bea0, SI group, stack value

void hs_evaluate_device_group_set_immediate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        device_group_set_value_immediate(*(uint16_t *)&arguments[0], *(float *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
