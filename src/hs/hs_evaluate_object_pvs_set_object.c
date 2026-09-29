// hs_evaluate_object_pvs_set_object  (not a Ghidra function; the evaluate handler of hs functions 84 "object_pvs_activate" (object -> void); 85 "object_pvs_set_object" (object -> void))
// address 0x47b5d0, size 103 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b5d0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b5d0..0x47b637: object_globals (0x006b8cbc) +0x90 word = 0 for a -1 object; otherwise +0x94 = the object and +0x90 = 1;
//   returns 0. Shared by object_pvs_activate.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern object_globals *object_globals_pointer;

void hs_evaluate_object_pvs_set_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index object_index = (datum_index)arguments[0];

    if (object_index == k_datum_index_none) {
        object_globals_pointer->ambient_cluster_mode = 0;
    } else {
        *(datum_index *)&object_globals_pointer->ambient_cluster_index = object_index;
        object_globals_pointer->ambient_cluster_mode = 1;
    }
    hs_thread_return(0, thread_index);
    }
}
