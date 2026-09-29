// hs_evaluate_net_graph_show  (not a Ghidra function; the evaluate handler of hs function 384 "net_graph_show" (string, string -> boolean))
// address 0x4806d0, size 89 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4806d0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4806d0..0x480728: returns network_bandwidth_graph_set_units_command (0x4d7d90)
//   for the two strings as a boolean (its low byte).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint32_t network_bandwidth_graph_set_units_command(const char *units_name, const char *direction_name); // 0x4d7d90, blam-cc: ECX, stack

void hs_evaluate_net_graph_show(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_thread_return((int32_t)(uint8_t)(network_bandwidth_graph_set_units_command((const char *)arguments[0], (const char *)arguments[1])), thread_index);
    }
}
