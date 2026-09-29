// hs_evaluate_hud_show_crosshair  (not a Ghidra function; the evaluate handler of hs function 402 "hud_show_crosshair" (boolean -> void))
// address 0x480c20, size 92 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480c20, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480c20..0x480c7c: sets (true) or clears (false) bit 0 of the weapon HUD globals (*0x00719430) +0x78; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint8_t *hud_weapon_state; // 0x00719430

void hs_evaluate_hud_show_crosshair(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (*(uint8_t *)&arguments[0]) {
        *(uint32_t *)(hud_weapon_state + 0x78) |= 1;
    } else {
        *(uint32_t *)(hud_weapon_state + 0x78) &= 0xfffffffe;
    }
    hs_thread_return(0, thread_index);
    }
}
