// hs_evaluate_unit_can_blink  (not a Ghidra function; the evaluate handler of hs function 93 "unit_can_blink" (unit, boolean -> void))
// address 0x47b810, size 130 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47b810 trapped.
// WRITTEN 2026-09-28 from objdump 0x47b810..0x47b891: unit_can_blink: true clears unit flag 0x400000 (+0x204),
//   false sets it; a none unit is ignored. Returns 0.
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
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern data_array *object_data; // 0x008603b0

void hs_evaluate_unit_can_blink(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1) {
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[arguments[0] & 0xffff].data;

            if (!(uint8_t)arguments[1]) {
                ((unit_object *)unit)->unit.flags |= 0x400000;
            } else {
                ((unit_object *)unit)->unit.flags &= ~0x400000u;
            }
        }
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
