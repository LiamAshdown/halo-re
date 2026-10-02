// network_bandwidth_graph_new_sample  (Ghidra: FUN_004d8430; named per
// out/phase2/results/networking_01.json / symbols/review_queue.txt)
// address 0x4d8430, size 145 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md summary ("Records a new bandwidth sample into
// a graph instance, updating its smoothed displayed rate and last-update timestamp");
// out/phase2/results/networking_01.json evidence ("calls
// network_bandwidth_graph_update_columns (0x4d81c0), averages the five most recent raw sample
// fields (+0x5d4..+0x5c4) into the displayed rate field +0x23dc, and stamps the current time
// (QueryPerformanceCounter scaled to ms via __allmul/__alldiv) into +4"); types/networking.h
// network_bandwidth_graph.history[320] (+0x5d4..+0x5c4 are history[319]..history[315]),
// .displayed_rate (+0x23dc), .last_sample_ms (+0x4), .pending_sample (+0xd0); the
// QueryPerformanceCounter/__allmul/__alldiv shape is the same one folded into plain int64_t
// arithmetic in src/math/random_seed_generate.c and src/networking/network_update.c.
// register convention: disassembly (`mov eax,[esi+0xd0]` then `push esi; call 0x4d81c0`, with
// no register saved/restored around the graph pointer beforehand) shows the graph pointer
// arrives in ESI (`unaff_ESI` in Ghidra's decompile) and is never reloaded from a stack slot.
// // blam-cc: ESI -> graph

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

extern void network_bandwidth_graph_update_columns(int32_t new_sample,
    network_bandwidth_graph *graph); // 0x4d81c0, this batch

// blam-cc: ESI -> graph
void network_bandwidth_graph_new_sample(network_bandwidth_graph *graph)
{
    large_integer counter;
    int32_t recent_sum;

    network_bandwidth_graph_update_columns(graph->pending_sample, graph);

    recent_sum = graph->history[319] + graph->history[318] + graph->history[317] +
                 graph->history[316] + graph->history[315];
    graph->displayed_rate = (float)recent_sum * 0.25f;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    graph->last_sample_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    graph->pending_sample = 0;
}

#if 0
Original Ghidra decompilation (0x4d8430):

void FUN_004d8430(void)

{
  undefined4 uVar1;
  int unaff_ESI;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  FUN_004d81c0();
  local_8.s.LowPart =
       *(int *)(unaff_ESI + 0x5d4) + *(int *)(unaff_ESI + 0x5d0) + *(int *)(unaff_ESI + 0x5cc) +
       *(int *)(unaff_ESI + 0x5c8) + *(int *)(unaff_ESI + 0x5c4);
  *(float *)(unaff_ESI + 0x23dc) = (float)(int)local_8.s.LowPart * 0.25;
  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(unaff_ESI + 4) = uVar1;
  *(undefined4 *)(unaff_ESI + 0xd0) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
