// hs_evaluate_object_set_collideable  (not a Ghidra function; the evaluate handler of hs "object_set_collideable" (object, boolean -> void))
// address 0x47b240, size 121 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47b240, only reachable through that pointer.
//   Campaign track: 11 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47b240: inline: object +0x10 bit 0x1000000 set when the boolean is false, cleared when true; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern data_array *object_data; // 0x008603b0

void hs_evaluate_object_set_collideable(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != 0xffffffff) {
            uint8_t *object = (uint8_t *)((object_header *)object_data->data)[arguments[0] & 0xffff].data;

            // object +0x10 bit 0x1000000 = not collideable: set when the boolean is false
            if (*(uint8_t *)&arguments[1] == 0) {
                *(uint32_t *)(object + 0x10) |= 0x1000000;
            } else {
                *(uint32_t *)(object + 0x10) &= 0xfeffffff;
            }
        }
        hs_thread_return(0, thread_index);
    }
}
