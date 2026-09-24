// network_bandwidth_graph_accumulate_received  (Ghidra: FUN_004d7a50; named per this rewrite)
// address 0x4d7a50, size 116 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Accumulates incoming (received) byte
// counts for the network bandwidth debug graph, updating the displayed total only when
// 'received' is the active view"); mirrors network_bandwidth_graph_accumulate_sent.c exactly,
// with direction_index == 1 and bits_received instead of direction_index == 0 and bits_sent.
// register convention: byte count in ESI (unaff_ESI, unresolved), packet count in the
// Ghidra-recognized stack parameter. // blam-cc: ESI -> byte_count, stack -> packet_count

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_bandwidth_graph network_bandwidth_graph_globals; // 0x00719ce0
extern int32_t time_query_performance_counter_ms(void); // 0x449210, tick counter, outside this batch

// blam-cc: ESI -> byte_count, stack -> packet_count
void network_bandwidth_graph_accumulate_received(int32_t byte_count, int32_t packet_count)
{
    if (network_bandwidth_graph_globals.direction_index == 1) {
        if (network_bandwidth_graph_globals.units_index == 0) {
            network_bandwidth_graph_globals.pending_sample += byte_count;
        } else if (network_bandwidth_graph_globals.units_index == 1) {
            network_bandwidth_graph_globals.pending_sample += packet_count;
        }
        if (network_bandwidth_graph_globals.needs_layout != 0) {
            network_bandwidth_graph_globals.last_sample_ms = time_query_performance_counter_ms();
            network_bandwidth_graph_globals.needs_layout = 0;
        }
    }
    network_bandwidth_graph_globals.bits_received += byte_count * 8;
    if (network_bandwidth_graph_globals.rate_base_ms == 0) {
        network_bandwidth_graph_globals.rate_base_ms = time_query_performance_counter_ms();
    }
}

#if 0
Original Ghidra decompilation (0x4d7a50):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004d7a50(int param_1)

{
  int unaff_ESI;

  if (DAT_00719cf0 == 1) {
    if (DAT_00719cec == 0) {
      DAT_00719db0 = DAT_00719db0 + unaff_ESI;
    }
    else if (DAT_00719cec == 1) {
      DAT_00719db0 = DAT_00719db0 + param_1;
    }
    if (DAT_00719ce0 != '\0') {
      _DAT_00719ce4 = FUN_00449210();
      DAT_00719ce0 = '\0';
    }
  }
  DAT_00719da0 = DAT_00719da0 + unaff_ESI * 8;
  if (DAT_00719da4 == 0) {
    DAT_00719da4 = FUN_00449210();
  }
  return;
}
#endif
