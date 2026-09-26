// hs_evaluate_hud_show_shield  (not a Ghidra function; the evaluate handler of hs function 398 "hud_show_shield" (boolean -> void))
// address 0x480aa0, size 92 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480aa0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480aa0..0x480afc: clears (true) or sets (false) bit 4 of the unit meter globals (*0x0071942c) +0x58; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *hud_unit_meters; // 0x0071942c

void hs_evaluate_hud_show_shield(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (*(uint8_t *)&arguments[0]) {
        *(uint32_t *)(hud_unit_meters + 0x58) &= 0xfffffffb;
    } else {
        *(uint32_t *)(hud_unit_meters + 0x58) |= 4;
    }
    hs_thread_return(0, thread_index);
    }
}
