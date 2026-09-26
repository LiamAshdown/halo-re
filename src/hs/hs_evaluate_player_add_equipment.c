// hs_evaluate_player_add_equipment  (not a Ghidra function; the evaluate handler of hs function 363 "player_add_equipment" (unit, starting_profile, boolean -> void))
// address 0x47f4b0, size 78 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f4b0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f4b0..0x47f4fe: unit_apply_starting_profile(AX profile, ECX unit, stack: the boolean zero-extended), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void unit_apply_starting_profile(int16_t starting_profile_index, datum_index unit_handle, uint8_t reset_stats);
    // 0x473c50, blam-cc: EAX, ECX, stack

void hs_evaluate_player_add_equipment(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    unit_apply_starting_profile(*(int16_t *)&arguments[1], (datum_index)arguments[0], *(uint8_t *)&arguments[2]);
    hs_thread_return(0, thread_index);
    }
}
