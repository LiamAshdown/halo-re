// network_bandwidth_graph_accumulate_sent  (Ghidra: FUN_004d79d0; named per this rewrite)
// address 0x4d79d0, size 116 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Accumulates outgoing (sent) byte
// counts for the network bandwidth debug graph, updating the displayed total only when 'sent'
// is the active view"); types/networking.h network_bandwidth_graph (units_index,
// direction_index, pending_sample, needs_layout, last_sample_ms, bits_sent, rate_base_ms).
// register convention: byte count in ESI (unaff_ESI, unresolved), packet count in the
// Ghidra-recognized stack parameter. // blam-cc: ESI -> byte_count, stack -> packet_count

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_bandwidth_graph network_bandwidth_graph_globals; // 0x00719ce0
extern int32_t time_query_performance_counter_ms(void); // 0x449210, tick counter, outside this batch

// blam-cc: ESI -> byte_count, stack -> packet_count
void network_bandwidth_graph_accumulate_sent(int32_t byte_count, int32_t packet_count)
{
    if (network_bandwidth_graph_globals.direction_index == 0) {
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
    network_bandwidth_graph_globals.bits_sent += byte_count * 8;
    if (network_bandwidth_graph_globals.rate_base_ms == 0) {
        network_bandwidth_graph_globals.rate_base_ms = time_query_performance_counter_ms();
    }
}

#if 0
Original Ghidra decompilation (0x4d79d0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004d79d0(int param_1)

{
  int unaff_ESI;

  if (DAT_00719cf0 == 0) {
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
  DAT_00719d9c = DAT_00719d9c + unaff_ESI * 8;
  if (DAT_00719da4 == 0) {
    DAT_00719da4 = FUN_00449210();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
