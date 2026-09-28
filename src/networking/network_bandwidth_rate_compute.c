// network_bandwidth_rate_compute  (Ghidra: network_bandwidth_rate_compute, already named)
// address 0x4d8540, size 212 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md summary ("Computes the smoothed sent/received
// bits-per-second rates for the network bandwidth debug overlay from accumulated byte counters
// and elapsed time"); types/networking.h network_bandwidth_graph.rate_base_ms (+0xc4),
// .bits_sent (+0xbc), .bits_received (+0xc0), .rate_sent (+0xc8), .rate_received (+0xcc); the
// QueryPerformanceCounter/__allmul/__alldiv shape is the same one folded into plain int64_t
// arithmetic in src/math/random_seed_generate.c and src/networking/network_update.c.
// register convention: Ghidra shows no recognized parameters and one `unaff_ESI`, matching
// every other function in this file's family (the graph pointer arrives in ESI and is never
// reloaded from the stack). // blam-cc: ESI -> graph
// UNSURE: when the elapsed time is under one second, the code inverts it (`elapsed = 1.0 /
// elapsed`) before taking `1.0 / elapsed` again in the final multiply, which nets out to
// *multiplying* the byte counters by the (sub-second) elapsed time instead of dividing by it --
// the opposite of a normal rate. This looks intentional (it damps the displayed rate instead
// of letting it spike when the sampling interval is very short) rather than a decompiler
// artifact, since both `__ftol` truncations round-trip through ordinary float ops with no sign
// or register game involved; preserved exactly either way, per the task's no-invented-behaviour
// rule.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, owned by the timing/system module
extern int32_t __stdcall QueryPerformanceCounter(large_integer *counter);

// Reinterprets a possibly-wrapped int32 millisecond/byte counter as unsigned, the way this
// function's disassembly does with an explicit "add 2^32 if negative" after the float convert.
static float as_unsigned_float(int32_t value)
{
    float result = (float)value;
    if (value < 0) {
        result = result + 4294967296.0f;
    }
    return result;
}

// blam-cc: ESI -> graph
void network_bandwidth_rate_compute(network_bandwidth_graph *graph)
{
    large_integer counter;
    int32_t base_ms = graph->rate_base_ms;
    int32_t now_ms;
    float elapsed_seconds;

    QueryPerformanceCounter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    elapsed_seconds = as_unsigned_float(now_ms - base_ms) * 0.001f;

    if (base_ms == 0) {
        graph->rate_sent = 0.0f;
        graph->rate_received = 0.0f;
        return;
    }

    if (elapsed_seconds < 1.0f) {
        elapsed_seconds = 1.0f / elapsed_seconds; // see file header UNSURE note
    }
    graph->rate_sent = as_unsigned_float(graph->bits_sent) * (1.0f / elapsed_seconds);
    graph->rate_received = as_unsigned_float(graph->bits_received) * (1.0f / elapsed_seconds);
}

#if 0
Original Ghidra decompilation (0x4d8540):

void network_bandwidth_rate_compute(void)

{
  int iVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int unaff_ESI;
  undefined8 uVar5;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  iVar1 = *(int *)(unaff_ESI + 0xc4);
  uVar5 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  iVar4 = __alldiv(uVar5,DAT_006ac8f8,DAT_006ac8fc);
  iVar4 = iVar4 - iVar1;
  fVar2 = (float)iVar4;
  if (iVar4 < 0) {
    fVar2 = fVar2 + 4.2949673e+09;
  }
  fVar2 = fVar2 * 0.001;
  if (iVar1 != 0) {
    if (fVar2 < 1.0) {
      fVar2 = 1.0 / fVar2;
    }
    fVar3 = (float)*(int *)(unaff_ESI + 0xbc);
    if (*(int *)(unaff_ESI + 0xbc) < 0) {
      fVar3 = fVar3 + 4.2949673e+09;
    }
    *(float *)(unaff_ESI + 200) = fVar3 * (1.0 / fVar2);
    fVar3 = (float)*(int *)(unaff_ESI + 0xc0);
    if (*(int *)(unaff_ESI + 0xc0) < 0) {
      fVar3 = fVar3 + 4.2949673e+09;
    }
    *(float *)(unaff_ESI + 0xcc) = fVar3 * (1.0 / fVar2);
    return;
  }
  *(undefined4 *)(unaff_ESI + 200) = 0;
  *(undefined4 *)(unaff_ESI + 0xcc) = 0;
  return;
}
#endif
