// hs_evaluate_vehicle_driver  (not a Ghidra function; the evaluate handler of hs function 122 "vehicle_driver" (unit -> unit))
// address 0x47c340, size 81 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47c340 trapped.
// WRITTEN 2026-09-28 from objdump 0x47c340..0x47c390: returns the driver (+0x324) of the unit's object
//   (object_try_and_get mask 3), -1 when it is not a unit.
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
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX, stack

void hs_evaluate_vehicle_driver(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t *unit = (uint8_t *)object_try_and_get((datum_index)arguments[0], 3);

        hs_thread_return(unit != 0 ? *(int32_t *)&((unit_object *)unit)->unit.driver_unit_index : -1, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
