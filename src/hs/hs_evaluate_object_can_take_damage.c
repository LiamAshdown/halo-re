// hs_evaluate_object_can_take_damage  (not a Ghidra function; the evaluate handler of hs function 80 "object_can_take_damage" (object_list -> void))
// address 0x47b470, size 63 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b470, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b470..0x47b4af: object_hash_clear_flag_bit3(EAX = the object list), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_objects.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_object_can_take_damage(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    object_hash_clear_flag_bit3((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}
