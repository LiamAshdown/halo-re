// hs_evaluate_object_set_permutation  (not a Ghidra function; the evaluate handler of hs "object_set_permutation" (object, string, string -> void))
// address 0x47a8b0, size 76 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47a8b0, only reachable through that pointer.
//   Campaign track: 32 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47a8b0: stack (object +0x0, permutation name +0x8), EBX = region name (+0x4); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void hs_object_set_permutation_by_name(datum_index object_index, void *param_2, char *name); // 0x488670,
    // stack (object, permutation name), EBX = region name

void hs_evaluate_object_set_permutation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_object_set_permutation_by_name((datum_index)arguments[0], (void *)arguments[2], (char *)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
