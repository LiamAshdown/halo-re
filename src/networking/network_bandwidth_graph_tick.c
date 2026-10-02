// network_bandwidth_graph_tick  (Ghidra: FUN_004d84d0; named per
// out/phase2/results/networking_01.json / symbols/review_queue.txt)
// address 0x4d84d0, size 100 bytes
// name confidence: 0.45   rewrite confidence: 0.65
// evidence: out/phase4/networking_functions.md summary ("Advances a network bandwidth-graph
// instance forward by any elapsed sampling intervals since it was last updated");
// out/phase2/results/networking_01.json evidence ("computes elapsed ms since the instance's
// last update (+4) via QueryPerformanceCounter, and while the elapsed time exceeds the
// sampling interval (+8) repeatedly calls network_bandwidth_graph_new_sample (0x4d8430),
// consuming the interval each time"); types/networking.h network_bandwidth_graph.needs_layout
// (+0x0), .last_sample_ms (+0x4), .sample_interval_ms (+0x8).
// register convention: disassembly (`mov esi,eax` as the first instruction) shows the graph
// pointer arrives in EAX (`in_EAX` in Ghidra's decompile).
// // blam-cc: EAX -> graph

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, owned by the timing/system module

extern void network_bandwidth_graph_new_sample(network_bandwidth_graph *graph); // 0x4d8430, this batch

// blam-cc: EAX -> graph
void network_bandwidth_graph_tick(network_bandwidth_graph *graph)
{
    large_integer counter;
    uint32_t elapsed_ms;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    if (graph->needs_layout != 0) {
        return;
    }

    elapsed_ms = (uint32_t)((counter.quad_part * 1000) / performance_frequency) -
                 (uint32_t)graph->last_sample_ms;
    while (graph->sample_interval_ms <= elapsed_ms) {
        network_bandwidth_graph_new_sample(graph);
        elapsed_ms -= graph->sample_interval_ms;
    }
}

#if 0
Original Ghidra decompilation (0x4d84d0):

void FUN_004d84d0(void)

{
  char *in_EAX;
  int iVar1;
  uint uVar2;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  if (*in_EAX == '\0') {
    uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    iVar1 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
    uVar2 = iVar1 - *(int *)(in_EAX + 4);
    if (*(uint *)(in_EAX + 8) <= uVar2) {
      do {
        FUN_004d8430();
        uVar2 = uVar2 - *(uint *)(in_EAX + 8);
      } while (*(uint *)(in_EAX + 8) <= uVar2);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
