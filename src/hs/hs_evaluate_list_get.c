// hs_evaluate_list_get  (not a Ghidra function; the evaluate handler of hs function 45 "list_get" (object_list, short -> object))
// address 0x47a900, size 65 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47a900, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47a900..0x47a941: returns object_list_nth_reference(EAX list, CX index).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern int32_t object_list_nth_reference(datum_index header_index, int16_t n); // 0x488570, blam-cc: EAX, ECX

void hs_evaluate_list_get(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return(object_list_nth_reference((datum_index)arguments[0], *(int16_t *)&arguments[1]), thread_index);
    }
}
