// hs_evaluate_object_set_melee_attack_inhibited  (not a Ghidra function; the evaluate handler of hs "object_set_melee_attack_inhibited" (object, boolean -> void))
// address 0x47b1b0, size 116 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47b1b0, only reachable through that pointer.
//   Campaign track: 1 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47b1b0: object +0x106 bit 0x80 set / cleared by the boolean; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern data_array *object_data; // 0x008603b0

void hs_evaluate_object_set_melee_attack_inhibited(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != 0xffffffff) {
            uint8_t *object = (uint8_t *)((object_header *)object_data->data)[arguments[0] & 0xffff].data;

            if (*(uint8_t *)&arguments[1] != 0) {
                object[0x106] |= 0x80;
            } else {
                object[0x106] &= 0x7f;
            }
        }
        hs_thread_return(0, thread_index);
    }
}
