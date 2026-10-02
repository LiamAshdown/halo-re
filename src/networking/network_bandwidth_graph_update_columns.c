// network_bandwidth_graph_update_columns  (Ghidra: FUN_004d81c0; named per
// out/phase2/results/networking_01.json / symbols/review_queue.txt)
// address 0x4d81c0, size 605 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("Feeds a new sample into a network
// bandwidth-graph instance and recomputes the interpolated column heights used to draw the
// scrolling line graph"); out/phase2/results/networking_01.json evidence ("shifts the +0xd8
// history window down by one and records the new sample (in_EAX) at +0x5d4; when the current
// time bucket (+0x23d8, refilled from network_bandwidth_graph_find_peak_sample at 0x4d8140
// every 0x140 frames) changes, rotates several 8-entry min/max history rows and recomputes the
// label float; otherwise recomputes interpolated per-column screen-height values across the
// visible window"); types/networking.h network_bandwidth_graph.history[320], .columns[320],
// .peak_scale (+0x23d8), .unknown_00d4 (+0xd4), .baseline (+0x28).
// register convention: disassembly of the prologue (`mov ebp,[esp+8]` before any push) shows
// `param_1` (the graph pointer) is the sole stack parameter, Ghidra-recognized; `in_EAX` (the
// new sample) is confirmed live at the one call site (network_bandwidth_graph_new_sample,
// 0x4d8430, this batch: `mov eax,[esi+0xd0]` i.e. graph->pending_sample, immediately before
// `push esi; call 0x4d81c0`).
// // blam-cc: EAX -> new_sample, stack -> graph
// The scale factor at graph+0x1c (inside networking.h's documented
// network_bandwidth_graph.unknown_0014 scratch range) is the value
// network_bandwidth_graph_instance_update_layout (0x4d7e20, already committed) computes and
// stores there; it has no header field of its own, so it is read by raw offset here too, for
// consistency with that file.
// Ghidra unrolls the "shift every column's y left by one" step as two interleaved loops (an
// 8-wide stride of 39 groups, plus a 7-iteration cleanup at the tail) built around 9-wide
// overlapping windows. Traced by hand: together they write exactly
// columns[i].y = columns[i+1].y for i in 0..318, in increasing order of i, which is a plain
// left shift with no aliasing hazard (each read of columns[i+1] happens before that slot is
// ever overwritten). Collapsed here into that equivalent loop; not a behavioural change.
// Likewise the "recompute every column from scratch" path is Ghidra's unrolled 32x10 loop over
// history[0..319], collapsed into one straight loop applying the same per-element formula.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t network_bandwidth_graph_find_peak_sample(int32_t *out_peak_countdown,
    network_bandwidth_graph *graph); // 0x4d8140, this batch

// blam-cc: EAX -> new_sample, stack -> graph
void network_bandwidth_graph_update_columns(int32_t new_sample, network_bandwidth_graph *graph)
{
    uint8_t *base = (uint8_t *)graph;
    float scale = *(float *)(base + 0x1c); // see file header
    int32_t old_peak_scale = graph->peak_scale;
    int32_t i;

    // Shift the 320-entry sample history left by one and append the new sample.
    for (i = 0; i < 319; i++) {
        graph->history[i] = graph->history[i + 1];
    }
    graph->history[319] = new_sample;

    if (new_sample < graph->peak_scale) {
        graph->peak_samples_remaining -= 1;
        if (graph->peak_samples_remaining == 0) {
            graph->peak_scale = network_bandwidth_graph_find_peak_sample(&graph->peak_samples_remaining, graph);
        }
    } else {
        graph->peak_scale = new_sample;
        graph->peak_samples_remaining = 0x140;
    }

    if (old_peak_scale == graph->peak_scale) {
        // Peak did not change: reuse the previous 319 column heights and shift them down
        // by one instead of recomputing every column from the raw history.
        for (i = 0; i < 319; i++) {
            graph->columns[i].y = graph->columns[i + 1].y;
        }
        graph->columns[319].y = (float)(int32_t)graph->baseline -
            ((float)graph->history[319] / (float)graph->peak_scale) * scale;
        return;
    }

    // Peak changed: recompute every column's height from the raw history.
    for (i = 0; i < 320; i++) {
        graph->columns[i].y = (float)(int32_t)graph->baseline -
            ((float)graph->history[i] / (float)graph->peak_scale) * scale;
    }
}

#if 0
Original Ghidra decompilation (0x4d81c0):

void FUN_004d81c0(int param_1)

{
  int in_EAX;
  undefined4 uVar1;
  int *piVar2;
  undefined4 *puVar3;
  int iVar4;
  float *pfVar5;
  int iVar6;
  undefined4 *puVar7;

  iVar6 = *(int *)(param_1 + 0x23d8);
  puVar3 = (undefined4 *)(param_1 + 0xdc);
  puVar7 = (undefined4 *)(param_1 + 0xd8);
  for (iVar4 = 0x13f; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar7 = *puVar3;
    puVar3 = puVar3 + 1;
    puVar7 = puVar7 + 1;
  }
  *(int *)(param_1 + 0x5d4) = in_EAX;
  if (in_EAX < *(int *)(param_1 + 0x23d8)) {
    iVar4 = *(int *)(param_1 + 0xd4) + -1;
    *(int *)(param_1 + 0xd4) = iVar4;
    if (iVar4 == 0) {
      uVar1 = FUN_004d8140(param_1);
      *(undefined4 *)(param_1 + 0x23d8) = uVar1;
    }
  }
  else {
    *(int *)(param_1 + 0x23d8) = in_EAX;
    *(undefined4 *)(param_1 + 0xd4) = 0x140;
  }
  if (iVar6 == *(int *)(param_1 + 0x23d8)) {
    puVar3 = (undefined4 *)(param_1 + 0x5f4);
    iVar6 = 0x27;
    do {
      puVar3[-6] = *puVar3;
      *puVar3 = puVar3[6];
      puVar3[6] = puVar3[0xc];
      puVar3[0xc] = puVar3[0x12];
      puVar3[0x12] = puVar3[0x18];
      puVar3[0x18] = puVar3[0x1e];
      puVar3[0x1e] = puVar3[0x24];
      puVar3[0x24] = puVar3[0x2a];
      puVar3 = puVar3 + 0x30;
      iVar6 = iVar6 + -1;
    } while (iVar6 != 0);
    puVar3 = (undefined4 *)(param_1 + 0x231c);
    iVar6 = 7;
    do {
      *puVar3 = puVar3[6];
      puVar3 = puVar3 + 6;
      iVar6 = iVar6 + -1;
    } while (iVar6 != 0);
    *(float *)(param_1 + 0x23c4) =
         (float)(int)*(short *)(param_1 + 0x28) -
         ((float)*(int *)(param_1 + 0x5d4) / (float)*(int *)(param_1 + 0x23d8)) *
         *(float *)(param_1 + 0x1c);
    return;
  }
  iVar6 = 0x20;
  piVar2 = (int *)(param_1 + 0xdc);
  pfVar5 = (float *)(param_1 + 0x5f4);
  do {
    pfVar5[-6] = (float)(int)*(short *)(param_1 + 0x28) -
                 ((float)piVar2[-1] / (float)*(int *)(param_1 + 0x23d8)) *
                 *(float *)(param_1 + 0x1c);
    *pfVar5 = (float)(int)*(short *)(param_1 + 0x28) -
              ((float)*piVar2 / (float)*(int *)(param_1 + 0x23d8)) * *(float *)(param_1 + 0x1c);
    pfVar5[6] = (float)(int)*(short *)(param_1 + 0x28) -
                ((float)piVar2[1] / (float)*(int *)(param_1 + 0x23d8)) * *(float *)(param_1 + 0x1c);
    pfVar5[0xc] = (float)(int)*(short *)(param_1 + 0x28) -
                  ((float)piVar2[2] / (float)*(int *)(param_1 + 0x23d8)) *
                  *(float *)(param_1 + 0x1c);
    pfVar5[0x12] = (float)(int)*(short *)(param_1 + 0x28) -
                   ((float)piVar2[3] / (float)*(int *)(param_1 + 0x23d8)) *
                   *(float *)(param_1 + 0x1c);
    pfVar5[0x18] = (float)(int)*(short *)(param_1 + 0x28) -
                   ((float)piVar2[4] / (float)*(int *)(param_1 + 0x23d8)) *
                   *(float *)(param_1 + 0x1c);
    pfVar5[0x1e] = (float)(int)*(short *)(param_1 + 0x28) -
                   ((float)piVar2[5] / (float)*(int *)(param_1 + 0x23d8)) *
                   *(float *)(param_1 + 0x1c);
    iVar6 = iVar6 + -1;
    pfVar5[0x24] = (float)(int)*(short *)(param_1 + 0x28) -
                   ((float)piVar2[6] / (float)*(int *)(param_1 + 0x23d8)) *
                   *(float *)(param_1 + 0x1c);
    pfVar5[0x2a] = (float)(int)*(short *)(param_1 + 0x28) -
                   ((float)piVar2[7] / (float)*(int *)(param_1 + 0x23d8)) *
                   *(float *)(param_1 + 0x1c);
    pfVar5[0x30] = (float)(int)*(short *)(param_1 + 0x28) -
                   ((float)piVar2[8] / (float)*(int *)(param_1 + 0x23d8)) *
                   *(float *)(param_1 + 0x1c);
    piVar2 = piVar2 + 10;
    pfVar5 = pfVar5 + 0x3c;
  } while (iVar6 != 0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
