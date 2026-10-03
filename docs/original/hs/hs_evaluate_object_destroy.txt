// hs_evaluate_object_destroy  (not a Ghidra function; the evaluate handler of hs function 39 "object_destroy" (object -> void))
// address 0x47a610, size 85 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47a610, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47a610..0x47a665: an object that is not -1 and not held by hs_object_hierarchy_test is deleted (EAX); returns 0.
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
extern char hs_object_hierarchy_test(datum_index object_index); // 0x487c10
extern void object_delete(datum_index object_index); // 0x4f5bd0, blam-cc: EAX

void hs_evaluate_object_destroy(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index object_index = (datum_index)arguments[0];

    if (object_index != k_datum_index_none && !hs_object_hierarchy_test(object_index)) {
        object_delete(object_index);
    }
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
