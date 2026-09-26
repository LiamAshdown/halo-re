// hs_evaluate_object_type_predict  (not a Ghidra function; the evaluate handler of hs function 83 "object_type_predict" (object_definition -> void))
// address 0x47b570, size 94 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b570, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b570..0x47b5ce: a tag other than -1 has its predicted resources (tag data +0x170) touched: predicted_resource_list_touch(ESI);
//   returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "cache.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern tag_instance *tag_instances; // 0x0087bc14
extern void predicted_resource_list_touch(TagReflexive *resources); // 0x4449f0, blam-cc: ESI

void hs_evaluate_object_type_predict(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index tag = (datum_index)arguments[0];

    if (tag != k_datum_index_none) {
        predicted_resource_list_touch((TagReflexive *)((uint8_t *)tag_instances[tag & 0xffff].data + 0x170));
    }
    hs_thread_return(0, thread_index);
    }
}
