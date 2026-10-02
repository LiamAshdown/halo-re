// hs_evaluate_sv_unban  (not a Ghidra function; the evaluate handler of hs function 499 "sv_unban" (long -> void))
// address 0x482a20, size 116 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482a20 trapped.
// WRITTEN 2026-09-28 from objdump 0x482a20..0x482a93: for a ban list index in range (0x006b859c, 0x38 byte
//   entries): prints "Unbanning %s." with the entry, removes it and saves the ban list; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void chimera__console_out(void *color, char *format, ...); // 0x496b50, blam-cc: EAX color
extern growable_array ban_list; // 0x006b859c
extern void growable_array_remove_element(growable_array *array, uint32_t index); // 0x4cf890, blam-cc: ESI, EDI
extern void network_banlist_save(void); // 0x4e3380

void hs_evaluate_sv_unban(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int32_t index = arguments[0];

        if (index >= 0 && index < ban_list.count) {
            chimera__console_out(0, (char *)"Unbanning %s.", (uint8_t *)ban_list.data + index * 0x38); // 0x0066d71c
            growable_array_remove_element(&ban_list, (uint32_t)index);
            network_banlist_save();
        }
        hs_thread_return(0, thread_index);
    }
}
