// network_bandwidth_graph_find_peak_sample  (Ghidra: FUN_004d8140; named per
// out/phase2/results/networking_01.json / symbols/review_queue.txt)
// address 0x4d8140, size 114 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md summary ("Finds the index of the largest
// sample in the bandwidth-graph history, used to rescale the graph's vertical axis");
// out/phase2/results/networking_01.json evidence ("scans the graph's sample array in groups
// of 5 across its full 0x140-entry length tracking the running maximum value and its index,
// writing the resulting index out through the caller-supplied pointer (unaff_EBX)");
// types/networking.h network_bandwidth_graph.history[320]. The one call site
// (network_bandwidth_graph_update_columns, 0x4d81c0, this batch) is disassembled directly
// (objdump -d -M intel --start-address=0x4d81c0) to confirm EBX really is live at the call:
// it is `lea ebx,[ebp+0xd4]` a few instructions earlier and never reloaded before `call
// 0x4d8140`, so the callee's conditional `*unaff_EBX = ...` genuinely writes back through it.
// register convention: Ghidra recognizes `param_1` (the graph pointer) as an ordinary
// argument; disassembly of the function prologue (`mov esi,[esp+8]` before any other push)
// confirms it is the sole stack parameter. `unaff_EBX` is a true incoming register argument
// (never pushed/popped inside the function), the optional out-pointer for the sample index.
// // blam-cc: EBX -> out_peak_countdown, stack -> graph
// Ghidra's grouped/unrolled comparisons (blocks of 5, stride 5) are collapsed here into an
// equivalent single increasing-index loop; verified by hand that the group boundaries always
// advance the compared index by exactly 1 each step (0,1,2,3,4, then 5,6,7,8,9, ...), so this
// is not a behavioural change.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

// blam-cc: EBX -> out_peak_countdown, stack -> graph
int32_t network_bandwidth_graph_find_peak_sample(int32_t *out_peak_countdown, network_bandwidth_graph *graph)
{
    int32_t peak_value = 1;   // sentinel: stays 1 if every sample is <= 0
    int32_t peak_index = 319; // default: the last history slot
    int32_t i;

    for (i = 0; i < 320; i++) {
        if (peak_value <= graph->history[i]) {
            peak_value = graph->history[i];
            peak_index = i;
        }
    }

    if (out_peak_countdown != 0) {
        // How many more samples until this peak scrolls out of the 320-entry window.
        *out_peak_countdown = peak_index + 1;
    }
    return peak_value;
}

#if 0
Original Ghidra decompilation (0x4d8140):

void FUN_004d8140(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int *unaff_EBX;
  int *piVar5;

  iVar2 = 1;
  iVar4 = 0x13f;
  piVar5 = (int *)(param_1 + 0xdc);
  iVar3 = 2;
  do {
    if (iVar2 <= piVar5[-1]) {
      iVar4 = iVar3 + -2;
      iVar2 = piVar5[-1];
    }
    if (iVar2 <= *piVar5) {
      iVar4 = iVar3 + -1;
      iVar2 = *piVar5;
    }
    if (iVar2 <= piVar5[1]) {
      iVar2 = piVar5[1];
      iVar4 = iVar3;
    }
    if (iVar2 <= piVar5[2]) {
      iVar4 = iVar3 + 1;
      iVar2 = piVar5[2];
    }
    if (iVar2 <= piVar5[3]) {
      iVar4 = iVar3 + 2;
      iVar2 = piVar5[3];
    }
    iVar1 = iVar3 + 3;
    piVar5 = piVar5 + 5;
    iVar3 = iVar3 + 5;
  } while (iVar1 < 0x140);
  if (unaff_EBX != (int *)0x0) {
    *unaff_EBX = iVar4 + 1;
  }
  return;
}
#endif
