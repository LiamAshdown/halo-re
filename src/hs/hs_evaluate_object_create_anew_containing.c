// hs_evaluate_object_create_anew_containing  (not a Ghidra function; the evaluate handler of hs function 42 "object_create_anew_containing" (string -> void))
// address 0x47a710, size 73 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47a710, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47a710..0x47a759: hs_object_names_for_each(stack hs_object_name_cache_validate 0x487d20, EBX = the string), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void hs_object_names_for_each(void (*callback)(int32_t index), uint32_t predicate_arg); // 0x487ef0, EBX, stack
extern void hs_object_name_cache_validate(int16_t object_name_index); // 0x487d20

void hs_evaluate_object_create_anew_containing(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_object_names_for_each((void (*)(int32_t))hs_object_name_cache_validate, (uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
