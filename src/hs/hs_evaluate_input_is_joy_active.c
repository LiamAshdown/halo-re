// hs_evaluate_input_is_joy_active  (not a Ghidra function; the evaluate handler of hs function 446 "input_is_joy_active" (short -> boolean))
// address 0x481820, size 109 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_481820 trapped.
// WRITTEN 2026-09-28 from objdump 0x481820..0x48188c: true when the device index is below the device count
//   (0x006b1844) and its slot (0x006b1a98 + device * 0x240) is assigned.
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
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern int32_t input_device_count; // 0x006b1844
extern uint8_t input_device_to_slot[]; // 0x006b1a98, stride 0x240

void hs_evaluate_input_is_joy_active(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int32_t device = (int16_t)arguments[0];
        uint8_t active = 0;

        if (device < input_device_count) {
            active = (uint8_t)(*(int32_t *)(input_device_to_slot + device * 0x240) != -1);
        }
        hs_thread_return((int32_t)(uint8_t)(active), thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
