// hs_evaluate_objects_detach  (not a Ghidra function; the evaluate handler of hs function 77 "objects_detach" (object, object -> void))
// address 0x47b390, size 113 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b390, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b390..0x47b401: with both objects set and the child's parent (+0x11c) the given parent:
//   object_snap_to_parent_marker_and_detach(stack child); returns 0.
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
extern data_array *object_data; // 0x008603b0
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index); // 0x4f6610

void hs_evaluate_objects_detach(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index parent = (datum_index)arguments[0];
    datum_index child = (datum_index)arguments[1];

    if (parent != k_datum_index_none && child != k_datum_index_none &&
        *(datum_index *)(*(uint8_t **)((uint8_t *)object_data->data + (child & 0xffff) * 0xc + 8) + 0x11c) == parent) {
        object_snap_to_parent_marker_and_detach(child);
    }
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
