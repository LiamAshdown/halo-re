// hs_evaluate_scenery_get_animation_time  (not a Ghidra function; the evaluate handler of hs function 89 "scenery_get_animation_time" (scenery -> short))
// address 0x47b700, size 83 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47b700 trapped.
// WRITTEN 2026-09-28 from objdump 0x47b700..0x47b752: returns the scenery's remaining animation frames (0x4fa9b0)
//   as a short.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint32_t object_animation_get_frames_remaining(uint32_t object_index); // 0x4fa9b0, blam-cc: EAX

void hs_evaluate_scenery_get_animation_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_thread_return((int32_t)(uint16_t)(object_animation_get_frames_remaining((uint32_t)arguments[0])), thread_index);
    }
}
