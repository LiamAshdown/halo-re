// hs_evaluate_net_graph_clear  (not a Ghidra function; the evaluate handler of hs function 383 "net_graph_clear" ( -> void))
// address 0x4806b0, size 23 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4806b0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4806b0..0x4806c6: resets the network bandwidth graph history (0x4d8080 on
//   0x00719ce0); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern uint8_t network_bandwidth_graph_globals[]; // 0x00719ce0
extern void network_bandwidth_graph_instance_history_reset(void *graph); // 0x4d8080, blam-cc: ESI

void hs_evaluate_net_graph_clear(int16_t function_index, uint32_t thread_index, char first)
{
    network_bandwidth_graph_instance_history_reset(network_bandwidth_graph_globals);
    hs_thread_return(0, thread_index);
}
