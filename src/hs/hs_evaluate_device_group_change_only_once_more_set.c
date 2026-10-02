// hs_evaluate_device_group_change_only_once_more_set  (not a Ghidra function; the evaluate handler of hs "device_group_change_only_once_more_set" (device_group, boolean -> void))
// address 0x47cde0, size 115 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47cde0, only reachable through that pointer.
//   Campaign track: 1 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47cde0: the group record +0x2: bit 1 = the boolean, bit 2 always cleared; returns 0.
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
extern data_array *device_groups; // 0x0087abf0, 8-byte records, flags byte at +0x2

void hs_evaluate_device_group_change_only_once_more_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t group = *(int16_t *)&arguments[0];

        if (group != -1) {
            uint8_t *record = (uint8_t *)device_groups->data + (uint16_t)group * 8;

            if (*(uint8_t *)&arguments[1] != 0) {
                record[2] |= 1;
            } else {
                record[2] &= 0xfe;
            }
            record[2] &= 0xfd;
        }
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
