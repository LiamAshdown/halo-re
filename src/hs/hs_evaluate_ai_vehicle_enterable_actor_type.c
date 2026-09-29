// hs_evaluate_ai_vehicle_enterable_actor_type  (not a Ghidra function; the evaluate handler of hs "ai_vehicle_enterable_actor_type" (unit, actor_type -> void))
// address 0x47e1e0, size 95 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47e1e0, only reachable through that pointer.
//   Campaign track: 12 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47e1e0: for a unit other than -1 the attention record (EDI) gets bit (actor type, +0x4) set in its +0xa word; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void *ai_object_attention_find_or_create(datum_index object_index); // 0x435900, EDI

void hs_evaluate_ai_vehicle_enterable_actor_type(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != 0xffffffff) {
            uint8_t *record = (uint8_t *)ai_object_attention_find_or_create((datum_index)arguments[0]);

            if (record != 0) {
                *(uint16_t *)(record + 0xa) |= (uint16_t)(1u << (*(uint8_t *)&arguments[1] & 0x1f));
            }
        }
        hs_thread_return(0, thread_index);
    }
}
