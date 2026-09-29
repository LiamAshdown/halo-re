// hs_evaluate_profile_load  (not a Ghidra function; the evaluate handler of hs function 505 "profile_load" (string -> void))
// address 0x4827a0, size 63 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4827a0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4827a0..0x4827de: saved_game_delete_by_display_name (0x53b9b0) with the name
//   (ECX); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void saved_game_delete_by_display_name(const char *name); // 0x53b9b0, blam-cc: ECX

void hs_evaluate_profile_load(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        saved_game_delete_by_display_name((const char *)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}
