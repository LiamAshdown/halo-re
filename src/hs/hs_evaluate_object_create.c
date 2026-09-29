// hs_evaluate_object_create  (not a Ghidra function; the evaluate handler of hs function 38 "object_create" (object_name -> void))
// address 0x47a5b0, size 96 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47a5b0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47a5b0..0x47a610: a name other than -1 whose slot is out of range or empty (object_name_list 0x006b8cb8) is created with
//   object_new_from_scenario_name (CX name); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern datum_index *object_name_list; // 0x006b8cb8
extern datum_index object_new_from_scenario_name(int16_t name_index); // 0x4f7370, blam-cc: CX

void hs_evaluate_object_create(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    int16_t name = *(int16_t *)&arguments[0];

    if (name != -1 && (name < 0 || name >= 0x200 || object_name_list[name] == k_datum_index_none)) {
        object_new_from_scenario_name(name);
    }
    hs_thread_return(0, thread_index);
    }
}
