// hs_evaluate_ai_detach  (not a Ghidra function; the evaluate handler of hs function 161 "ai_detach" (unit -> void))
// address 0x47d140, size 107 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47d140, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47d140..0x47d1ab: the unit's actor (+0x1f4), when there is one, is deleted: actor_delete(EBX actor, stack 0); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern data_array *object_data; // 0x008603b0
extern void actor_delete(datum_index actor_index, uint32_t flag); // 0x427e60, blam-cc: EBX, stack

void hs_evaluate_ai_detach(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];

    if (unit != k_datum_index_none) {
        datum_index actor = *(datum_index *)(*(uint8_t **)((uint8_t *)object_data->data + (unit & 0xffff) * 0xc + 8) + 0x1f4);

        if (actor != k_datum_index_none) {
            actor_delete(actor, 0);
        }
    }
    hs_thread_return(0, thread_index);
    }
}
