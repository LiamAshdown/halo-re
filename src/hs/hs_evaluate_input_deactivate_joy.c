// hs_evaluate_input_deactivate_joy  (not a Ghidra function; the evaluate handler of hs function 448 "input_deactivate_joy" (short -> void))
// address 0x481920, size 107 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_481920 trapped.
// WRITTEN 2026-09-28 from objdump 0x481920..0x48198a: for a device index below the count with an assigned slot:
//   unassigns it (device slot and joystick_slot_devices[slot] = -1); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern int32_t input_device_count; // 0x006b1844
extern uint8_t input_device_to_slot[]; // 0x006b1a98, stride 0x240
extern int32_t joystick_slot_devices[4]; // 0x006b2ce8

void hs_evaluate_input_deactivate_joy(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int32_t device = (int16_t)arguments[0];

        if (device < input_device_count) {
            int32_t *slot = (int32_t *)(input_device_to_slot + device * 0x240);

            if (*slot != -1) {
                int32_t old_slot = *slot;

                *slot = -1;
                joystick_slot_devices[old_slot] = -1;
            }
        }
        hs_thread_return(0, thread_index);
    }
}
