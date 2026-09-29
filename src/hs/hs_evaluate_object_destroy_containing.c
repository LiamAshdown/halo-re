// hs_evaluate_object_destroy_containing  (not a Ghidra function; the evaluate handler of hs function 43 "object_destroy_containing" (string -> void))
// address 0x47a760, size 73 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47a760, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47a760..0x47a7a9: hs_object_names_for_each(stack hs_object_name_destroy 0x487d90, EBX = the string), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void hs_object_names_for_each(void (*callback)(int32_t index), uint32_t predicate_arg); // 0x487ef0, EBX, stack


void hs_evaluate_object_destroy_containing(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_object_names_for_each(hs_object_name_destroy, (uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}
