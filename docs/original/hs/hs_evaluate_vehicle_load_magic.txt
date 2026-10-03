// hs_evaluate_vehicle_load_magic  (not a Ghidra function; the evaluate handler of hs function 116 "vehicle_load_magic" (unit, string, object_list -> short))
// address 0x47c150, size 94 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47c150, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47c150..0x47c1ae: unit_seat_candidates_from_zone_and_enter(stack: unit, name; EAX list) as a word in a zeroed dword.
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
extern int16_t unit_seat_candidates_from_zone_and_enter(uint32_t unit_index, char *name_filter, uint32_t zone_list_index);
    // 0x56a4c0, blam-cc: stack, stack, EAX

void hs_evaluate_vehicle_load_magic(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)(uint16_t)unit_seat_candidates_from_zone_and_enter((uint32_t)arguments[0],
        (char *)arguments[1], (uint32_t)arguments[2]), thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
