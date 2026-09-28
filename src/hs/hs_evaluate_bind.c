// hs_evaluate_bind  (not a Ghidra function; the evaluate handler of hs function 481 "bind" (string, string, string -> void))
// address 0x4823e0, size 74 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4823e0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4823e0..0x482429: hs_bind_control (0x48b750) with the device class, input and
//   action names; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void hs_bind_control(const char *device_class_name, const char *input_name, const char *action_name); // 0x48b750, blam-cc: EAX device, stack input, action

void hs_evaluate_bind(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_bind_control((const char *)arguments[0], (const char *)arguments[1], (const char *)arguments[2]);
        hs_thread_return(0, thread_index);
    }
}
