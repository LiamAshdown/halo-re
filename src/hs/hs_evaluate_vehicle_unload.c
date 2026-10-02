// hs_evaluate_vehicle_unload  (not a Ghidra function; the evaluate handler of hs "vehicle_unload"
//   (unit, string -> short))
// address 0x47c1b0, size 91 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47c1b0, only reachable through that pointer.
//   Campaign track: 145 uses across the campaign scripts (not a10); it had no C and would have trapped.
// objdump 0x47c1b0..0x47c20a: arguments (unit +0x0, seat label string +0x4) go to
//   unit_detach_child_at_named_seat (cdecl); its short result is returned zero-extended (the return slot is
//   cleared before the word store at 0x47c1f4).
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
extern int16_t unit_detach_child_at_named_seat(uint32_t unit_index, char *seat_marker_name); // 0x56ab50, cdecl

void hs_evaluate_vehicle_unload(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t count = unit_detach_child_at_named_seat((uint32_t)arguments[0], (char *)arguments[1]);

        hs_thread_return((int32_t)(uint16_t)count, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
