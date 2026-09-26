// hs_evaluate_hud_set_timer_position  (not a Ghidra function; the evaluate handler of hs function 408 "hud_set_timer_position" (short, short, hud_corner -> void))
// address 0x480e00, size 133 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480e00, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480e00..0x480e85: HUD messaging (*0x006b3a40) +0x480/+0x482 = x/y; +0x484 = the corner clamped to [0, 4]; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *hud_messaging; // 0x006b3a40

void hs_evaluate_hud_set_timer_position(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    int16_t corner = *(int16_t *)&arguments[2];

    *(int16_t *)(hud_messaging + 0x480) = *(int16_t *)&arguments[0];
    *(int16_t *)(hud_messaging + 0x482) = *(int16_t *)&arguments[1];
    *(int16_t *)(hud_messaging + 0x484) = corner < 0 ? 0 : (corner > 4 ? 4 : corner);
    hs_thread_return(0, thread_index);
    }
}
