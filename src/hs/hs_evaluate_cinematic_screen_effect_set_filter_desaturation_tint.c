// hs_evaluate_cinematic_screen_effect_set_filter_desaturation_tint  (not a Ghidra function; the evaluate handler of hs function 425 "cinematic_screen_effect_set_filter_desaturation_tint" (real, real, real -> void))
// address 0x481280, size 97 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x481280, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x481280..0x4812e1: the screen effect state (*0x0071cfc4), when present, gets +0x14/+0x18/+0x1c = the tint; returns 0.
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
extern uint8_t *cinematic_screen_effect_state; // 0x0071cfc4

void hs_evaluate_cinematic_screen_effect_set_filter_desaturation_tint(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (cinematic_screen_effect_state != 0) {
        *(int32_t *)(cinematic_screen_effect_state + 0x18) = arguments[1];
        *(int32_t *)(cinematic_screen_effect_state + 0x14) = arguments[0];
        *(int32_t *)(cinematic_screen_effect_state + 0x1c) = arguments[2];
    }
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
