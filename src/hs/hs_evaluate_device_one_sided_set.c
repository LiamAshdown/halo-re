// hs_evaluate_device_one_sided_set  (not a Ghidra function; the evaluate handler of hs "device_one_sided_set" (device, boolean -> void))
// address 0x47cce0, size 122 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47cce0, only reachable through that pointer.
//   Campaign track: 17 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47cce0: object_try_and_get(device, mask 0x80); the boolean sets or clears device +0x214 bit 2; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack

void hs_evaluate_device_one_sided_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t *device = (uint8_t *)object_try_and_get((datum_index)arguments[0], 0x80);

        if (device != 0) {
            if (*(uint8_t *)&arguments[1] != 0) {
                *(uint32_t *)(device + 0x214) |= 2;
            } else {
                *(uint32_t *)(device + 0x214) &= 0xfffffffd;
            }
        }
        hs_thread_return(0, thread_index);
    }
}
