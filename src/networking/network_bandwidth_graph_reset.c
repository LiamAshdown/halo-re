// network_bandwidth_graph_reset  (Ghidra: FUN_004d7980; named per this rewrite)
// address 0x4d7980, size 74 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md summary ("Resets the global network
// bandwidth-graph debug overlay's counters and mode selectors to their default state");
// types/networking.h network_bandwidth_graph and network_bandwidth_graph_globals /
// network_bandwidth_graph_default_interval_ms.
// register convention: __cdecl (or no-arg helper), no parameters recognized by Ghidra.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern network_bandwidth_graph network_bandwidth_graph_globals; // 0x00719ce0
extern uint32_t network_bandwidth_graph_default_interval_ms;    // 0x006894b0


uint32_t network_bandwidth_graph_reset(void)
{
    network_bandwidth_graph_globals.last_sample_ms = 0;
    network_bandwidth_graph_globals.units_index = 0;
    network_bandwidth_graph_globals.bits_sent = 0;
    network_bandwidth_graph_globals.bits_received = 0;
    network_bandwidth_graph_globals.rate_base_ms = 0;
    network_bandwidth_graph_globals.needs_layout = 1;
    network_bandwidth_graph_globals.sample_interval_ms = network_bandwidth_graph_default_interval_ms;
    network_bandwidth_graph_globals.direction_index = 1;
    network_bandwidth_graph_instance_update_layout(&network_bandwidth_graph_globals, 1);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4d7980):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_004d7980(void)

{
  _DAT_00719ce4 = 0;
  DAT_00719cec = 0;
  DAT_00719d9c = 0;
  DAT_00719da0 = 0;
  DAT_00719da4 = 0;
  DAT_00719ce0 = 1;
  DAT_00719ce8 = DAT_006894b0;
  DAT_00719cf0 = 1;
  FUN_004d7e20(1);
  return 1;
}
#endif
