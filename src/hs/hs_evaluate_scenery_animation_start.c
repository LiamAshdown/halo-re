// hs_evaluate_scenery_animation_start  (not a Ghidra function; the evaluate handler of hs function 90
//   "scenery_animation_start" (scenery, animation_graph, string -> void))
// address 0x47b760, size 76 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[90] -> record, evaluate (+0xc) 0x47b760, only reachable through
//   that pointer. Campaign track: used 11 times across the campaign scripts (not by a10); it had no C and
//   would have trapped (unlisted callback).
// objdump 0x47b760..0x47b7ab: arguments (object +0x0, graph tag +0x4, name +0x8) go to object_start_animation
//   (EAX object, EDI graph, ECX name, stack frame 0); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void object_start_animation(uint32_t object_index, datum_index graph_tag, char *name,
    int16_t requested_frame); // 0x4fa8d0, EAX object, EDI graph, ECX name, stack frame

void hs_evaluate_scenery_animation_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        object_start_animation((uint32_t)arguments[0], (datum_index)arguments[1], (char *)arguments[2], 0);
        hs_thread_return(0, thread_index);
    }
}
