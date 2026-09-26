// hs_evaluate_ai_retreat  (not a Ghidra function; the evaluate handler of hs functions 180 "ai_retreat" (ai -> void); 181 "ai_maneuver" (ai -> void))
// address 0x47d760, size 63 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47d760, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47d760..0x47d79f: ai_platoon_range_set_unknown_01(EAX ai), returns 0. Shared by ai_maneuver.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void ai_platoon_range_set_unknown_01(uint32_t packed_reference); // 0x4332e0, blam-cc: EAX

void hs_evaluate_ai_retreat(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    ai_platoon_range_set_unknown_01((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}
