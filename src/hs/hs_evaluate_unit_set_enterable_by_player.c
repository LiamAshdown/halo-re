// hs_evaluate_unit_set_enterable_by_player  (not a Ghidra function; the evaluate handler of hs "unit_set_enterable_by_player" (unit, boolean -> void))
// address 0x47bdb0, size 130 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47bdb0, only reachable through that pointer.
//   Campaign track: 35 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47bdb0: inline: a unit other than -1 gets +0x204 bit 0x10000 set when the boolean (+0x4) is false, cleared when true; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern data_array *object_data; // 0x008603b0

void hs_evaluate_unit_set_enterable_by_player(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != 0xffffffff) {
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[arguments[0] & 0xffff].data;

            // unit +0x204 bit 0x10000 blocks the player: set when the boolean is false
            if (*(uint8_t *)&arguments[1] == 0) {
                ((unit_object *)unit)->unit.flags |= 0x10000;
            } else {
                ((unit_object *)unit)->unit.flags &= 0xfffeffff;
            }
        }
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
