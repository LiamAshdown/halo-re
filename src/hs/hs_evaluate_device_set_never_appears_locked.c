// hs_evaluate_device_set_never_appears_locked  (not a Ghidra function; the evaluate handler of hs "device_set_never_appears_locked" (device, boolean -> void))
// address 0x47c8b0, size 127 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47c8b0, only reachable through that pointer.
//   Campaign track: 8 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47c8b0: a device other than -1 (object_try_and_get mask 0x80) gets +0x214 bit 4 set / cleared by the boolean; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack

void hs_evaluate_device_set_never_appears_locked(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != 0xffffffff) {
            uint8_t *device = (uint8_t *)object_try_and_get((datum_index)arguments[0], 0x80);

            if (device != 0) {
                if (*(uint8_t *)&arguments[1] != 0) {
                    *(uint32_t *)(device + 0x214) |= 4;
                } else {
                    *(uint32_t *)(device + 0x214) &= 0xfffffffb;
                }
            }
        }
        hs_thread_return(0, thread_index);
    }
}
