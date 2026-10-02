// hs_evaluate_recording_play_and_hover  (not a Ghidra function; the evaluate handler of hs "recording_play_and_hover" (unit, cutscene_recording -> boolean))
// address 0x47b010, size 93 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47b010, only reachable through that pointer.
//   Campaign track: 49 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47b010: EAX = unit (+0x0), CX = recording (+0x4 word), stack 0x10 (hover); the byte result is returned zero-extended.
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
extern uint8_t recorded_animation_start(datum_index unit_index, int16_t scenario_animation_index,
    uint16_t extra_flags); // 0x44a930, EAX unit, CX recording, stack flags

void hs_evaluate_recording_play_and_hover(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t result = recorded_animation_start((datum_index)arguments[0], (int16_t)*(uint16_t *)&arguments[1], 0x10);
        hs_thread_return((int32_t)result, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
