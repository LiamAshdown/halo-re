// hs_evaluate_player_effect_set_max_translation  (not a Ghidra function; the evaluate handler of hs function 391 "player_effect_set_max_translation" (real, real, real -> void))
// address 0x480810, size 88 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480810, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480810..0x480868: player effect globals (*0x006f1884) +0x100/+0x104/+0x108 = the three values, returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *player_effect_globals_pointer; // 0x006f1884

void hs_evaluate_player_effect_set_max_translation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    *(int32_t *)(player_effect_globals_pointer + 0x104) = arguments[1];
    *(int32_t *)(player_effect_globals_pointer + 0x100) = arguments[0];
    *(int32_t *)(player_effect_globals_pointer + 0x108) = arguments[2];
    hs_thread_return(0, thread_index);
    }
}
