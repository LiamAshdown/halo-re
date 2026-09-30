// hs_evaluate_change_team  (not a Ghidra function; the evaluate handler of hs function 485 "change_team" (short -> void))
// address 0x482490, size 69 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482490 trapped.
// WRITTEN 2026-09-28 from objdump 0x482490..0x4824d4: sends the team allegiance message (0x4704d0) with the low
//   byte of the short, zero-extended; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_game.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_change_team(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        game_engine_send_team_allegiance_message((char)(uint8_t)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}
