// hs_evaluate_object_beautify  (not a Ghidra function; the evaluate handler of hs function 81 "object_beautify" (object, boolean -> void))
// address 0x47b4b0, size 123 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b4b0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b4b0..0x47b52b: sets (true) or clears (false) bit 0x400000 of the object's flags (+0x10); returns 0.
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

void hs_evaluate_object_beautify(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index object_index = (datum_index)arguments[0];

    if (object_index != k_datum_index_none) {
        uint32_t *flags = (uint32_t *)(*(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8) + 0x10);

        if (*(uint8_t *)&arguments[1]) {
            *flags |= 0x400000;
        } else {
            *flags &= 0xffbfffff;
        }
    }
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
