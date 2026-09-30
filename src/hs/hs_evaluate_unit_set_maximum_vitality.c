// hs_evaluate_unit_set_maximum_vitality  (not a Ghidra function; the evaluate handler of hs function 112 "unit_set_maximum_vitality" (unit, real, real -> void))
// address 0x47bfe0, size 128 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47bfe0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47bfe0..0x47c060: a unit other than -1 whose object is not frozen (+0x106 bit 2) gets
//   object_initialize_shield_stun_thresholds(EAX unit, ESI &body, EDI &shield); returns 0 either way.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_objects.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern data_array *object_data; // 0x008603b0


void hs_evaluate_unit_set_maximum_vitality(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];
    float body = *(float *)&arguments[1];
    float shield = *(float *)&arguments[2];

    if (unit != k_datum_index_none &&
        (*(*(uint8_t **)((uint8_t *)object_data->data + (unit & 0xffff) * 0xc + 8) + 0x106) & 4) == 0) {
        object_initialize_shield_stun_thresholds(unit, &body, &shield);
    }
    hs_thread_return(0, thread_index);
    }
}
