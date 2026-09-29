// hs_evaluate_ai_disregard  (not a Ghidra function; the evaluate handler of hs "ai_disregard" (object_list, boolean -> void))
// address 0x47db50, size 72 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47db50, only reachable through that pointer.
//   Campaign track: 39 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47db50: EAX = object list (+0x0), stack the boolean (+0x4 byte, zero-extended); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void ai_object_list_set_unit_flag_400(datum_index object_list_header_handle, char flag); // 0x4347b0, EAX, stack

void hs_evaluate_ai_disregard(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_object_list_set_unit_flag_400((datum_index)arguments[0], *(char *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
