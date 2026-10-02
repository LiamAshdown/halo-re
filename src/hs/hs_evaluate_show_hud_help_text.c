// hs_evaluate_show_hud_help_text  (not a Ghidra function; the evaluate handler of hs function 365 "show_hud_help_text" (boolean -> boolean))
// address 0x4802b0, size 86 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x4802b0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x4802b0..0x480306: the HUD flags (*0x00719420) +0x01 = the boolean, returned in a zeroed dword.
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
extern uint8_t *hud_flags; // 0x00719420

void hs_evaluate_show_hud_help_text(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t show = *(uint8_t *)&arguments[0];

    hud_flags[1] = show;
    hs_thread_return((int32_t)show, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
