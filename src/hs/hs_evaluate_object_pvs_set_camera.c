// hs_evaluate_object_pvs_set_camera  (not a Ghidra function; the evaluate handler of hs function 86 "object_pvs_set_camera" (cutscene_camera_point -> void))
// address 0x47b640, size 64 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b640, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b640..0x47b680: objects_set_ambient_cluster_override(AX = the camera point word), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void objects_set_ambient_cluster_override(int16_t local_player_index); // 0x4f79d0, blam-cc: AX

void hs_evaluate_object_pvs_set_camera(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    objects_set_ambient_cluster_override(*(int16_t *)&arguments[0]);
    hs_thread_return(0, thread_index);
    }
}
