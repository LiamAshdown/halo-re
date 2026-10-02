// network_bandwidth_graph_instance_history_reset  (Ghidra: FUN_004d8080; named per this rewrite)
// address 0x4d8080, size 183 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Resets a network bandwidth-graph
// instance's sample ring buffer and history array to an empty starting state"); every field
// this function touches matches types/networking.h network_bandwidth_graph exactly
// (columns[].color/x/y, bits_sent/bits_received/rate_base_ms/rate_sent/rate_received/
// pending_sample/unknown_00d4, history[320], peak_scale, displayed_rate, last_sample_ms).
// register convention: graph instance in ESI (unaff_ESI, unresolved).
//   // blam-cc: ESI -> graph

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

// blam-cc: ESI -> graph
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void network_bandwidth_graph_instance_history_reset(network_bandwidth_graph *graph)
{
    int16_t baseline = graph->baseline;
    int16_t right = graph->right;
    int16_t left = graph->left;
    int32_t accumulator = 0;
    int32_t i;

    for (i = 0; i < 320; i++) {
        int32_t x_step = accumulator / 320;

        graph->columns[i].color = 0xffffffff;
        accumulator = accumulator + ((int32_t)right - (int32_t)left);
        graph->columns[i].x = (float)(x_step + left);
        graph->columns[i].y = (float)(int32_t)baseline;
    }

    graph->pending_sample = 0;
    for (i = 0; i < 320; i++) {
        graph->history[i] = 0;
    }
    graph->peak_scale = 1;
    graph->displayed_rate = 0.0f;
    graph->last_sample_ms = 0;
    graph->bits_received = 0;
    graph->bits_sent = 0;
    graph->rate_received = 0.0f;
    graph->rate_sent = 0.0f;
    graph->rate_base_ms = 0;
    graph->needs_layout = 1;
}

#if 0
Original Ghidra decompilation (0x4d8080):

void FUN_004d8080(void)

{
  short sVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar6;
  undefined1 *unaff_ESI;
  int iVar7;

  sVar1 = *(short *)(unaff_ESI + 0x28);
  sVar2 = *(short *)(unaff_ESI + 0x2a);
  sVar3 = *(short *)(unaff_ESI + 0x26);
  iVar7 = 0;
  iVar6 = 0x140;
  puVar5 = (undefined4 *)(unaff_ESI + 0x5e4);
  do {
    iVar4 = iVar7 / 0x140;
    *puVar5 = 0xffffffff;
    iVar7 = iVar7 + ((int)sVar2 - (int)sVar3);
    iVar6 = iVar6 + -1;
    puVar5[-3] = (float)(iVar4 + sVar3);
    puVar5[-2] = (float)(int)sVar1;
    puVar5 = puVar5 + 6;
  } while (iVar6 != 0);
  *(undefined4 *)(unaff_ESI + 0xd0) = 0;
  puVar5 = (undefined4 *)(unaff_ESI + 0xd8);
  for (iVar6 = 0x140; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  *(undefined4 *)(unaff_ESI + 0x23d8) = 1;
  *(undefined4 *)(unaff_ESI + 0x23dc) = 0;
  *(undefined4 *)(unaff_ESI + 4) = 0;
  *(undefined4 *)(unaff_ESI + 0xc0) = 0;
  *(undefined4 *)(unaff_ESI + 0xbc) = 0;
  *(undefined4 *)(unaff_ESI + 0xcc) = 0;
  *(undefined4 *)(unaff_ESI + 200) = 0;
  *(undefined4 *)(unaff_ESI + 0xc4) = 0;
  *unaff_ESI = 1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
