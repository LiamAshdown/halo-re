// hs_evaluate_camera_set_dead  (not a Ghidra function; the evaluate handler of hs "camera_set_dead" (unit -> void))
// address 0x47ef90, size 84 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47ef90, only reachable through that pointer.
//   Campaign track: 1 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47ef90: a unit other than -1: director mode 0x6869d2 = 3 (dead camera), byte 0x6869d1 = 1, target 0x686a04 = the unit; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern int16_t director_camera_mode; // 0x006869d2
extern uint8_t unknown_006869d1; // 0x006869d1
extern datum_index director_camera_target; // 0x00686a04

void hs_evaluate_camera_set_dead(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != 0xffffffff) {
            director_camera_mode = 3;
            unknown_006869d1 = 1;
            director_camera_target = (datum_index)arguments[0];
        }
        hs_thread_return(0, thread_index);
    }
}
