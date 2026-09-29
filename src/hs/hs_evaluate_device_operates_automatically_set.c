// hs_evaluate_device_operates_automatically_set  (not a Ghidra function; the evaluate handler of hs function 146 "device_operates_automatically_set" (device, boolean -> void))
// address 0x47cd60, size 122 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47cd60, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47cd60..0x47cdda: a device (object_try_and_get(ECX, 0x80)) gets bit 0 of +0x214 cleared for true and set for false; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX, stack

void hs_evaluate_device_operates_automatically_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t *device = (uint8_t *)object_try_and_get((datum_index)arguments[0], 0x80);

    if (device != 0) {
        if (*(uint8_t *)&arguments[1]) {
            *(uint32_t *)(device + 0x214) &= 0xfffffffe;
        } else {
            *(uint32_t *)(device + 0x214) |= 1;
        }
    }
    hs_thread_return(0, thread_index);
    }
}
