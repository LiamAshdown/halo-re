// hs_evaluate_camera_set  (not a Ghidra function; the evaluate handler of hs function 247 "camera_set")
// address 0x47ee40, size 76 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[247] -> record 0x659520, name "camera_set", return
//   type 4, parameters (13 cutscene_camera_point, 7 short); evaluate (+0xc) 0x47ee40, only
//   reachable through that pointer. First-boot track: ui.map's scripts call it 11 times.
// objdump 0x47ee40..0x47ee8b: the standard builtin evaluator (hs_evaluate_typed_arguments with
//   the definition's parameters); with arguments, camera_debug_start(AX = the point word, stack =
//   the tick word zero-extended, -1) and returns 0 (hs_thread_return, ECX = thread).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_camera.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


    // 0x444c00, blam-cc: AX -> camera_point_index, stack -> (ticks, relative_object)

void hs_evaluate_camera_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        camera_debug_start(*(int16_t *)&arguments[0], *(int16_t *)&arguments[1], k_datum_index_none);
        hs_thread_return(0, thread_index);
    }
}
