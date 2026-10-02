// hs_evaluate_camera_set_animation  (not a Ghidra function; the evaluate handler of hs function 249 "camera_set_animation" (animation_graph, string -> void))
// address 0x47eee0, size 72 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47eee0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47eee0..0x47ef27: camera_script_set_animation (0x444b30) with the animation
//   graph and the name; returns 0.
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
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void camera_script_set_animation(datum_index animation_tag, char *name); // 0x444b30, blam-cc: EBX tag, stack name

void hs_evaluate_camera_set_animation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        camera_script_set_animation((datum_index)arguments[0], (char *)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
