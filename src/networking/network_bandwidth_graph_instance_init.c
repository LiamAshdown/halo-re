// network_bandwidth_graph_instance_init  (Ghidra: network_bandwidth_graph_instance_init,
// already named)
// address 0x4d7de0, size 58 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md summary ("Initializes a per-instance network
// bandwidth-graph object with the given units/direction mode and forces its layout to be
// recomputed"); types/networking.h network_bandwidth_graph.needs_layout (+0x0),
// .last_sample_ms (+0x4), .sample_interval_ms (+0x8), .units_index (+0xc),
// .direction_index (+0x10), .bits_sent (+0xbc), .bits_received (+0xc0), .rate_base_ms (+0xc4).
// register convention: disassembly of the one call site (network_bandwidth_graph_set_units_
// command, 0x4d7d90, this batch: `push eax` [direction index] then `push esi` [units index]
// then `mov eax,0x719ce0; call`) shows the graph pointer arrives in EAX (`in_EAX` in Ghidra's
// decompile) and the two Ghidra-recognized stack parameters are, in push order, units index
// first (param_1) then direction index (param_2).
// // blam-cc: EAX -> graph, stack -> units_index, direction_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint32_t network_bandwidth_graph_default_interval_ms; // 0x006894b0

extern void network_bandwidth_graph_instance_update_layout(network_bandwidth_graph *graph,
    uint8_t force_refresh); // 0x4d7e20, already committed

// blam-cc: EAX -> graph, stack -> units_index, direction_index
void network_bandwidth_graph_instance_init(network_bandwidth_graph *graph, int32_t units_index,
    int32_t direction_index)
{
    graph->needs_layout = 1;
    graph->last_sample_ms = 0;
    graph->sample_interval_ms = network_bandwidth_graph_default_interval_ms;
    graph->units_index = units_index;
    graph->direction_index = direction_index;
    graph->bits_sent = 0;
    graph->bits_received = 0;
    graph->rate_base_ms = 0;
    network_bandwidth_graph_instance_update_layout(graph, 1);
}

#if 0
Original Ghidra decompilation (0x4d7de0):

void network_bandwidth_graph_instance_init(undefined4 param_1,undefined4 param_2)

{
  undefined1 *in_EAX;

  *in_EAX = 1;
  *(undefined4 *)(in_EAX + 4) = 0;
  *(undefined4 *)(in_EAX + 8) = DAT_006894b0;
  *(undefined4 *)(in_EAX + 0xc) = param_1;
  *(undefined4 *)(in_EAX + 0x10) = param_2;
  *(undefined4 *)(in_EAX + 0xbc) = 0;
  *(undefined4 *)(in_EAX + 0xc0) = 0;
  *(undefined4 *)(in_EAX + 0xc4) = 0;
  FUN_004d7e20(1);
  return;
}
#endif
