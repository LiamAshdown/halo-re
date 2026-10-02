// hs_evaluate_volume_test_object  (not a Ghidra function; the evaluate handler of hs "volume_test_object" (trigger_volume, object -> boolean))
// address 0x47a470, size 123 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47a470, only reachable through that pointer.
//   Campaign track: 10 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47a470: AX = the volume (+0x0), ECX = the object's bounding centre (+0xa0); false for no object; byte result zero-extended.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern data_array *object_data; // 0x008603b0
extern uint8_t scenario_trigger_volume_contains_point(int16_t trigger_volume_index, real_point3d *point); // 0x53f020, EAX, ECX

void hs_evaluate_volume_test_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t inside = 0;

        if ((uint32_t)arguments[1] != 0xffffffff) {
            uint8_t *object = (uint8_t *)((object_header *)object_data->data)[arguments[1] & 0xffff].data;

            inside = scenario_trigger_volume_contains_point(*(int16_t *)&arguments[0], (real_point3d *)(object + 0xa0));
        }
        hs_thread_return((int32_t)inside, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
