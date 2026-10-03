# Original notes: networking `net1_bandwidth`

Author notes and decompile blocks moved verbatim from the C files converted into the net1_bandwidth sources.

## network_bandwidth_direction_name_to_index.c

```
// network_bandwidth_direction_name_to_index  (Ghidra: FUN_004d8a50; renamed, no prior name)
// address 0x4d8a50, size 44 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md summary ("Converts a direction-name string (e.g.
// 'sent'/'recv') into the corresponding index for the network bandwidth debug graph"); identical
// shape to network_bandwidth_unit_name_to_index (0x4d8a20) one function above it, indexing
// network_bandwidth_direction_label_table, already named from
// src/networking/network_bandwidth_graph_instance_update_layout.c and
// src/networking/network_bandwidth_graph_update.c (0x0065d430, indexed by
// network_bandwidth_graph::direction_index). The two extra globals Ghidra lists
// (0x0066c2f0, 0x0066c2e8) are not referenced by this function's decompiled body and are
// omitted here.
// register convention: the string to match arrives in EDI (unaff_EDI). // blam-cc: EDI -> name

// VERIFIED against disassembly 0x4d8a50..0x4d8a7c (2026-09-30): _stricmp(name, label[i]) over two labels, -1 if none
```

```
#if 0
Original Ghidra decompilation (0x4d8a50):

int FUN_004d8a50(void)

{
  int iVar1;
  int iVar2;
  char *unaff_EDI;

  iVar2 = 0;
  do {
    iVar1 = __stricmp(unaff_EDI,(&PTR_DAT_0065d430)[iVar2]);
    if (iVar1 == 0) {
      return iVar2;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 2);
  return -1;
}
#endif
```

## network_bandwidth_graph_accumulate_received.c

```
// network_bandwidth_graph_accumulate_received  (Ghidra: FUN_004d7a50; named per this rewrite)
// address 0x4d7a50, size 116 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Accumulates incoming (received) byte
// counts for the network bandwidth debug graph, updating the displayed total only when
// 'received' is the active view"); mirrors network_bandwidth_graph_accumulate_sent.c exactly,
// with direction_index == 1 and bits_received instead of direction_index == 0 and bits_sent.
// register convention: byte count in ESI (unaff_ESI, unresolved), packet count in the
// Ghidra-recognized stack parameter. // blam-cc: ESI -> byte_count, stack -> packet_count
```

```
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
```

## network_bandwidth_graph_accumulate_sent.c

```
// network_bandwidth_graph_accumulate_sent  (Ghidra: FUN_004d79d0; named per this rewrite)
// address 0x4d79d0, size 116 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Accumulates outgoing (sent) byte
// counts for the network bandwidth debug graph, updating the displayed total only when 'sent'
// is the active view"); types/networking.h network_bandwidth_graph (units_index,
// direction_index, pending_sample, needs_layout, last_sample_ms, bits_sent, rate_base_ms).
// register convention: byte count in ESI (unaff_ESI, unresolved), packet count in the
// Ghidra-recognized stack parameter. // blam-cc: ESI -> byte_count, stack -> packet_count
```

```
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
```

## network_bandwidth_graph_find_peak_sample.c

```
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
```

```
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
```

## network_bandwidth_graph_instance_history_reset.c

```
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
```

```
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
```

## network_bandwidth_graph_instance_init.c

```
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
```

```
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
```

## network_bandwidth_graph_instance_update_layout.c

```
// network_bandwidth_graph_instance_update_layout  (Ghidra: FUN_004d7e20; named per this rewrite)
// address 0x4d7e20, size 596 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Recomputes the on-screen layout and
// label text of one network bandwidth-graph instance object when the screen size changes or a
// refresh is forced"); types/networking.h network_bandwidth_graph for the named fields
// (width/height at +0x14/+0x18 fall inside its documented unknown_0014[0x12], the label/layout
// scratch touched here falls inside its documented unknown_002c[0x90]); this function's own
// summary's naming hints ("update_for_resolution_change") and symbols/functions.txt
// rasterizer_resize_game_window for the 0x0069c634/0x0069c638 screen client-area corners.
// register convention: graph instance in EAX (in_EAX, unresolved), force-refresh flag in
// (Ghidra-recognized) param_1. // blam-cc: EAX -> graph, stack -> force_refresh
// TYPES-GAP: the four float constants this function multiplies by (0x672ab8, 0x672bc8,
// 0x672ac4, 0x672abc) and the two (0x672ba0, 0x672b9c) it divides by are not named anywhere;
// read directly from bin/halo.exe's .rdata (0.2, 0.4, 1.0, 0.5, 640.0, 480.0) since Ghidra folded
// only the first two into literals in its own decompile.
// UNSURE: every `__ftol()` call in Ghidra's decompile of this function is shown with an empty
// argument list -- __ftol takes its argument on the x87 stack (ST(0)), which Ghidra's decompiler
// frequently fails to attribute to a source expression. Every value below was reconstructed by
// tracing the x87 stack by hand against `objdump -d -M intel --start-address=0x4d7e20
// --stop-address=0x4d8080 bin/halo.exe`; the two computed-but-apparently-unread label-quad
// texture-scale expressions (R1, R2 and the fields at +0x2c/+0x2e/+0x34..+0x3e) are especially
// uncertain in *purpose* even though the arithmetic itself is confirmed against the disassembly.
// UNSURE: the label text is snprintf'd to `(uint8_t *)graph + 0x23e0`, which is past the end of
// types/networking.h's declared network_bandwidth_graph (size 0x23e0) -- the real object this
// function operates on is at least 0x25e0 bytes (0x23e0 struct + a 0x200-byte label buffer).
// Since types/*.h cannot be edited, the label buffer is reached by raw offset from the struct
// pointer rather than through a (missing) named field.

// VERIFIED against disassembly 0x4d7e20..0x4d8074 (2026-09-30): FIXED (same defects as network_bandwidth_graph_update: br.x/br.y swap, 640/480 divisors, _snprintf)
```

```
#if 0
Original Ghidra decompilation (0x4d7e20):

void FUN_004d7e20(char param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  undefined2 uVar5;
  undefined2 uVar6;
  int in_EAX;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  undefined4 *puVar11;
  float *pfVar12;

  iVar9 = (int)DAT_0069c638._2_2_;
  iVar10 = (int)(short)DAT_0069c638;
  iVar8 = iVar9 - DAT_0069c634._2_2_;
  iVar7 = iVar10 - (short)DAT_0069c634;
  if (((*(int *)(in_EAX + 0x14) != iVar7) || (*(int *)(in_EAX + 0x18) != iVar8)) ||
     (param_1 != '\0')) {
    *(int *)(in_EAX + 0x18) = iVar8;
    *(int *)(in_EAX + 0x14) = iVar7;
    *(float *)(in_EAX + 0x1c) = (float)iVar7 * 0.2;
    *(float *)(in_EAX + 0x20) = (float)iVar8 * 0.4;
    fVar2 = (float)(iVar10 + -0x40);
    fVar1 = (float)(iVar9 + -0x40);
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x26) = uVar5;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x24) = uVar5;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x2a) = uVar5;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x28) = uVar5;
    puVar11 = (undefined4 *)(in_EAX + 0x5d8);
    for (iVar9 = 0x780; iVar9 != 0; iVar9 = iVar9 + -1) {
      *puVar11 = 0;
      puVar11 = puVar11 + 1;
    }
    pfVar12 = (float *)(in_EAX + 0x44);
    for (iVar9 = 0x1e; iVar9 != 0; iVar9 = iVar9 + -1) {
      *pfVar12 = 0.0;
      pfVar12 = pfVar12 + 1;
    }
    FUN_004d8080();
    fVar3 = (fVar1 - (float)iVar8 * 0.4) - 1.0;
    *(undefined4 *)(in_EAX + 0x50) = 0xffffff00;
    *(undefined4 *)(in_EAX + 0x68) = 0xffffff00;
    *(float *)(in_EAX + 0x44) = fVar3;
    *(undefined4 *)(in_EAX + 0x80) = 0xffffff00;
    *(undefined4 *)(in_EAX + 0x98) = 0xffffff00;
    fVar4 = (fVar2 - (float)iVar7 * 0.2) - 1.0;
    *(undefined4 *)(in_EAX + 0xb0) = 0xffffff00;
    *(float *)(in_EAX + 0x48) = fVar4;
    fVar1 = fVar1 + 1.0;
    *(float *)(in_EAX + 0x5c) = fVar1;
    *(float *)(in_EAX + 0x60) = fVar4;
    *(float *)(in_EAX + 0x74) = fVar1;
    fVar2 = fVar2 + 1.0;
    *(float *)(in_EAX + 0x78) = fVar2;
    *(float *)(in_EAX + 0x8c) = fVar3;
    *(float *)(in_EAX + 0x90) = fVar2;
    *(float *)(in_EAX + 0xa4) = fVar3;
    *(float *)(in_EAX + 0xa8) = fVar4;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x2e) = uVar5;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x2c) = uVar5;
    *(undefined2 *)(in_EAX + 0x32) = 0x280;
    *(undefined2 *)(in_EAX + 0x30) = 0x1e0;
    uVar6 = __ftol();
    *(undefined2 *)(in_EAX + 0x36) = uVar6;
    *(undefined2 *)(in_EAX + 0x34) = uVar5;
    *(undefined2 *)(in_EAX + 0x3a) = 0x280;
    *(undefined2 *)(in_EAX + 0x38) = 0x1e0;
    *(undefined2 *)(in_EAX + 0x3e) = uVar6;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x3c) = uVar5;
    *(undefined2 *)(in_EAX + 0x42) = 0x280;
    *(undefined2 *)(in_EAX + 0x40) = 0x1e0;
    __snprintf((char *)(in_EAX + 0x23e0),0x200,"%s %s",
               (&PTR_s_bytes_0065d428)[*(int *)(in_EAX + 0xc)],
               (&PTR_DAT_0065d430)[*(int *)(in_EAX + 0x10)]);
  }
  return;
}
#endif
```

## network_bandwidth_graph_new_sample.c

```
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
```

```
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
```

## network_bandwidth_graph_reset.c

```
// network_bandwidth_graph_reset  (Ghidra: FUN_004d7980; named per this rewrite)
// address 0x4d7980, size 74 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md summary ("Resets the global network
// bandwidth-graph debug overlay's counters and mode selectors to their default state");
// types/networking.h network_bandwidth_graph and network_bandwidth_graph_globals /
// network_bandwidth_graph_default_interval_ms.
// register convention: __cdecl (or no-arg helper), no parameters recognized by Ghidra.
```

```
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
```

## network_bandwidth_graph_set_units_command.c

```
// network_bandwidth_graph_set_units_command  (Ghidra: FUN_004d7d90; named per
// out/phase2/results/networking_01.json / symbols/review_queue.txt)
// address 0x4d7d90, size 73 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md summary ("Parses a units/direction
// command-line pair (e.g. 'bytes sent') and, if valid, (re)configures the network bandwidth
// debug graph to display them"); out/phase2/results/networking_01.json evidence ("only
// proceeds if the overlay is enabled (DAT_00710305), resolves a units token via
// network_bandwidth_unit_name_to_index (0x4d8a20, matches 'bytes'/'packets') and a direction
// token via network_bandwidth_direction_name_to_index (0x4d8a50), and if both parse, calls
// network_bandwidth_graph_instance_init (0x4d7de0) with them"); types/networking.h
// network_bandwidth_graph_globals.
// register convention: disassembly (`mov edi,ecx` before the first call, no incoming push of
// ecx) shows the units-name string arrives in ECX; the direction-name string is the sole
// Ghidra-recognized stack parameter (`mov edi,[esp+0x10]`, read after two pushes). Both
// 0x4d8a20 and 0x4d8a50 are outside this batch (addresses above 0x4d8620); their own
// disassembly (`push edi` immediately before their shared string-compare callee, with no
// earlier setup of edi in either function) shows they read their one argument from EDI, so
// each of this function's two calls first moves its string pointer into EDI.
// // blam-cc: ECX -> units_name, stack -> direction_name
```

```
#if 0
Original Ghidra decompilation (0x4d7d90):

undefined4 FUN_004d7d90(void)

{
  int iVar1;
  int iVar2;

  if (DAT_00710305 == '\0') {
    return 0;
  }
  iVar1 = network_bandwidth_unit_name_to_index();
  iVar2 = FUN_004d8a50();
  if ((iVar1 != -1) && (iVar2 != -1)) {
    network_bandwidth_graph_instance_init(iVar1,iVar2);
    return 1;
  }
  return 0;
}
#endif
```

## network_bandwidth_graph_tick.c

```
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
```

```
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
```

## network_bandwidth_graph_update.c

```
// network_bandwidth_graph_update  (Ghidra: FUN_004d7ad0; named per this rewrite)
// address 0x4d7ad0, size 701 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Recomputes the on-screen layout and
// label of the global network bandwidth debug graph whenever the screen size changes, then
// redraws it"); every field this function touches is the SAME layout this batch's
// network_bandwidth_graph_instance_update_layout.c (0x4d7e20) computes for a per-instance
// graph, just inlined against the global singleton's absolute addresses instead of taking a
// pointer -- 0x00719cf4 is exactly network_bandwidth_graph_globals + 0x14, and the final
// snprintf destination 0x0071c0c0 is exactly network_bandwidth_graph_globals + 0x23e0, the same
// "past the declared struct" label buffer documented there. Cross-checking the two functions'
// field offsets against each other is itself part of the evidence both are transcribed
// correctly. See that file's header for the objdump-verified __ftol() argument reconstruction
// this one reuses.
// register convention: __cdecl, no parameters.
// TYPES-GAP: see network_bandwidth_graph_instance_update_layout.c for the label buffer past
// the struct's declared end. The screen client-area corners are types/networking.h's
// network_screen_point.

// VERIFIED against disassembly 0x4d7ad0..0x4d7d8d (2026-09-30): FIXED: right_raw/baseline_raw had br.x/br.y swapped (orig: +0x24 = (br.x-0x40)-W*0.2, +0x26 = (br.y-0x40)-H*0.4, +0x28 = br.x-0x40, +0x2a = br.y-0x40); 640/480 divisors are the raw height/width, not the scaled ones; snprintf(0x200) not sprintf; stray +0x12 store removed; network_stats_overlay_draw takes the graph in ESI
```

```
#if 0
Original Ghidra decompilation (0x4d7ad0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004d7ad0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  undefined2 uVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;

  if (DAT_00710305 != '\0') {
    iVar7 = (int)DAT_0069c638._2_2_ - (int)DAT_0069c634._2_2_;
    iVar6 = (int)(short)DAT_0069c638 - (int)(short)DAT_0069c634;
    if ((DAT_00719cf4 != iVar6) || (DAT_00719cf8 != iVar7)) {
      _DAT_00719cfc = (float)iVar6 * 0.2;
      _DAT_00719d00 = (float)iVar7 * 0.4;
      fVar3 = (float)((short)DAT_0069c638 + -0x40);
      fVar4 = fVar3 - _DAT_00719cfc;
      fVar2 = (float)(DAT_0069c638._2_2_ + -0x40);
      fVar1 = fVar2 - _DAT_00719d00;
      DAT_00719cf4 = iVar6;
      DAT_00719cf8 = iVar7;
      DAT_00719d06 = __ftol();
      DAT_00719d04 = __ftol();
      DAT_00719d0a = __ftol();
      _DAT_00719d08 = __ftol();
      puVar8 = &DAT_0071a2b8;
      for (iVar6 = 0x780; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      puVar8 = &DAT_00719d24;
      for (iVar6 = 0x1e; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      FUN_004d8080();
      DAT_00719d24 = fVar1 - 1.0;
      _DAT_00719d30 = 0xffffff00;
      _DAT_00719d48 = 0xffffff00;
      _DAT_00719d60 = 0xffffff00;
      _DAT_00719d78 = 0xffffff00;
      DAT_00719d28 = fVar4 - 1.0;
      _DAT_00719d90 = 0xffffff00;
      _DAT_00719d3c = fVar2 + 1.0;
      _DAT_00719d58 = fVar3 + 1.0;
      _DAT_00719d40 = DAT_00719d28;
      _DAT_00719d54 = _DAT_00719d3c;
      _DAT_00719d6c = DAT_00719d24;
      _DAT_00719d70 = _DAT_00719d58;
      _DAT_00719d84 = DAT_00719d24;
      _DAT_00719d88 = DAT_00719d28;
      _DAT_00719d0e = __ftol();
      uVar5 = __ftol();
      _DAT_00719d12 = 0x280;
      _DAT_00719d10 = 0x1e0;
      _DAT_00719d0c = uVar5;
      _DAT_00719d16 = __ftol();
      _DAT_00719d1a = 0x280;
      _DAT_00719d18 = 0x1e0;
      _DAT_00719d14 = uVar5;
      _DAT_00719d1e = _DAT_00719d16;
      _DAT_00719d1c = __ftol();
      _DAT_00719d22 = 0x280;
      _DAT_00719d20 = 0x1e0;
      __snprintf(&DAT_0071c0c0,0x200,"%s %s",(&PTR_s_bytes_0065d428)[DAT_00719cec],
                 (&PTR_DAT_0065d430)[DAT_00719cf0]);
    }
    network_stats_overlay_draw();
  }
  return;
}
#endif
```

## network_bandwidth_graph_update_columns.c

```
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
```

```
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
```

## network_bandwidth_rate_compute.c

```
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
```

```
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
```

## network_bandwidth_unit_name_to_index.c

```
// network_bandwidth_unit_name_to_index  (Ghidra: network_bandwidth_unit_name_to_index,
// already named)
// address 0x4d8a20, size 44 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md summary ("Converts a 'bytes'/'packets' unit-name
// string into the corresponding index for the network bandwidth debug graph"); the two strings
// referenced ("bytes", "packets") match network_bandwidth_units_label_table, already named from
// src/networking/network_bandwidth_graph_instance_update_layout.c and
// src/networking/network_bandwidth_graph_update.c (0x0065d428, indexed by
// network_bandwidth_graph::units_index).
// register convention: the string to match arrives in EDI (unaff_EDI). // blam-cc: EDI -> name
// UNSURE: none.
```

```
#if 0
Original Ghidra decompilation (0x4d8a20):

int network_bandwidth_unit_name_to_index(void)

{
  int iVar1;
  int iVar2;
  char *unaff_EDI;

  iVar2 = 0;
  do {
    iVar1 = __stricmp(unaff_EDI,(&PTR_s_bytes_0065d428)[iVar2]);
    if (iVar1 == 0) {
      return iVar2;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 2);
  return -1;
}
#endif
```

## network_stats_overlay_draw.c

```
// network_stats_overlay_draw  (Ghidra: network_stats_overlay_draw, already named)
// address 0x4d8620, size 1021 bytes
// name confidence: 0.6   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Renders the network bandwidth debug
// overlay: draws the scrolling line graph via the Direct3D device and overlays the
// sent/received bits-per-second text"); types/networking.h network_bandwidth_graph.columns
// (+0x5d8), .peak_scale (+0x23d8), .displayed_rate (+0x23dc), .rate_sent (+0xc8),
// .rate_received (+0xcc), the label text at +0x23e0 (see network_bandwidth_graph_instance_
// update_layout, 0x4d7e20, already committed, for why that offset has no header field).
// Ghidra's own decompile of every indirect call through the renderer's vtable
// (`(**(code **)(*DAT_0071d174 + N))()`) is corrupted here -- it shows fabricated locals
// holding literal return addresses instead of real arguments, the same class of failure noted
// in src/structures/structure_picked_polygon_draw.c. This rewrite instead reconstructs every
// vtable call from `objdump -d -M intel --start-address=0x4d8620 --stop-address=0x4d8a20`,
// matching the `void **device = *rasterizer_device; (*(fn**)((uint8_t*)device+offset))(...)`
// idiom that file already established for the same global. Ordinary direct calls (sprintf,
// chimera__draw_8_bit_text, rasterizer_set_shader_stage_config, hud_text_draw_configure, __ftol) were
// decompiled correctly by Ghidra and are taken from its output as-is (folding __ftol into a
// plain int cast, per the same pattern already used in
// network_bandwidth_graph_instance_update_layout).
// register convention: Ghidra shows only `unaff_ESI`, matching every other function in this
// bandwidth-graph family (the graph singleton arrives in ESI). // blam-cc: ESI -> graph
// vtable offsets 0xe4 and 0x10c are corroborated by other phase-2 batches as SetRenderState-
// style (state, value) and SetSamplerState-style (sampler, type, value) calls
// (out/phase2/results/rasterizer_00.json, render_01.json); 0x14c is corroborated as a
// DrawPrimitive-style call against a raw vertex array (render_01.json), and this batch's own
// evidence (primitive type 3, count 0x13f matching a 320-point line strip, vertex pointer
// exactly graph->columns, stride 0x18 matching sizeof(network_graph_vertex)) confirms it here
// independently. 0x134 is hedged elsewhere as a "SetTextureStageState/blend-style" toggle
// taking a single flag derived from DAT_0069c680 (out/phase2/results/rasterizer_03.json),
// which matches this function's two calls to it exactly.
// UNSURE: vtable offsets 0x15c, 0x170, 0x178, 0x104 and 0x1ac have no corroborating evidence
// anywhere in this repository; they are preserved as raw offset calls with their exact
// arguments rather than guessed names.
// UNSURE: DAT_006e1af0, DAT_006e1af8, DAT_0069e468 and the two packed-point globals at
// 0x7c1254/0x7c1258 are foreign renderer/UI state with no name recoverable from this module;
// declared as opaque externs.
// UNSURE: hud_text_draw_configure is tentatively "hud_meter_set_active_flash_color" per
// symbols/review_queue.txt (confidence 0.3); called here for a side effect unrelated to the
// text color this function sets immediately afterward (DAT_006e4738.. is overwritten
// unconditionally right after the call, per Ghidra's reliable decompile of that part).
// `network_screen_point` (types/networking.h) is the same packed {x,y} int16 pair used for
// game_window_top_left/game_window_bottom_right in network_bandwidth_graph_instance_update_
// layout, but this is a different pair of globals (0x7c1254/0x7c1258) with no confirmed owner.
// reconciled: R36 0x006e4738..0x006e4744 is ColorARGB text_color, alpha first: externs renamed r/g/b/a -> alpha/red/green/blue by address (same bytes)
```

```
#if 0
Original Ghidra decompilation (0x4d8620):

void network_stats_overlay_draw(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int unaff_ESI;
  char acStack_330 [4];
  int *piStack_32c;
  undefined4 uStack_328;
  undefined4 uStack_324;
  undefined4 uStack_320;
  int *piStack_31c;
  undefined4 uStack_318;
  undefined4 uStack_314;
  undefined4 uStack_310;
  int *piStack_30c;
  undefined4 uStack_308;
  undefined4 uStack_304;
  undefined4 uStack_300;
  int *piStack_2fc;
  undefined4 uStack_2f8;
  undefined4 uStack_2f4;
  undefined4 uStack_2f0;
  int *piStack_2ec;
  undefined4 uStack_2e8;
  undefined4 uStack_2e4;
  int *piStack_2e0;
  undefined4 uStack_2dc;
  undefined4 uStack_2d8;
  int *piStack_2d4;
  undefined4 uStack_2d0;
  undefined4 uStack_2cc;
  int *piStack_2c8;
  undefined4 uStack_2c4;
  undefined4 uStack_2c0;
  int *piStack_2bc;
  undefined4 uStack_2b8;
  undefined4 uStack_2b4;
  int *piStack_2b0;
  undefined4 uStack_2ac;
  undefined4 uStack_2a8;
  int *piStack_2a4;
  undefined4 uStack_2a0;
  undefined4 uStack_29c;
  int *piStack_298;
  undefined4 uStack_294;
  int *piStack_290;
  undefined4 uStack_28c;
  undefined1 *puStack_288;
  undefined4 uStack_284;
  int *piStack_280;
  int iStack_27c;
  int *piStack_278;
  uint uStack_274;
  int *piStack_270;
  undefined4 uStack_26c;

  uStack_26c = DAT_006e1af0;
  piStack_270 = DAT_0071d174;
  uStack_274 = 0x4d863c;
  (**(code **)(*DAT_0071d174 + 0x15c))();
  uStack_274 = -(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1af8 & 0x10;
  piStack_278 = DAT_0071d174;
  iStack_27c = 0x4d8663;
  (**(code **)(*DAT_0071d174 + 0x134))();
  iStack_27c = DAT_0069e468;
  piStack_280 = DAT_0071d174;
  uStack_284 = 0x4d8678;
  (**(code **)(*DAT_0071d174 + 0x170))();
  iStack_27c = (int)(short)((short)DAT_007c1258 - (short)DAT_007c1254);
  uStack_284 = 5;
  puStack_288 = &stack0xfffffd98;
  uStack_28c = 0xd;
  piStack_290 = DAT_0071d174;
  uStack_294 = 0x4d8778;
  (**(code **)(*DAT_0071d174 + 0x178))();
  uStack_294 = 0;
  piStack_298 = DAT_0071d174;
  uStack_29c = 0x4d8788;
  (**(code **)(*DAT_0071d174 + 0x1ac))();
  uStack_29c = 0x4d878f;
  rasterizer_set_shader_stage_config();
  uStack_29c = 1;
  uStack_2a0 = 0x16;
  piStack_2a4 = DAT_0071d174;
  uStack_2a8 = 0x4d87a1;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2a8 = 0xf;
  uStack_2ac = 0xa8;
  piStack_2b0 = DAT_0071d174;
  uStack_2b4 = 0x4d87b6;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2b4 = 0;
  uStack_2b8 = 0x1b;
  piStack_2bc = DAT_0071d174;
  uStack_2c0 = 0x4d87c8;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2c0 = 0;
  uStack_2c4 = 0xf;
  piStack_2c8 = DAT_0071d174;
  uStack_2cc = 0x4d87da;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2cc = 0;
  uStack_2d0 = 7;
  piStack_2d4 = DAT_0071d174;
  uStack_2d8 = 0x4d87ec;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2d8 = 0;
  uStack_2dc = 0xe;
  piStack_2e0 = DAT_0071d174;
  uStack_2e4 = 0x4d87fe;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2e4 = 0;
  uStack_2e8 = 0x1c;
  piStack_2ec = DAT_0071d174;
  uStack_2f0 = 0x4d8810;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2f0 = 3;
  uStack_2f4 = 1;
  uStack_2f8 = 0;
  piStack_2fc = DAT_0071d174;
  uStack_300 = 0x4d8824;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_300 = 0;
  uStack_304 = 3;
  uStack_308 = 0;
  piStack_30c = DAT_0071d174;
  uStack_310 = 0x4d8838;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_310 = 3;
  uStack_314 = 4;
  uStack_318 = 0;
  piStack_31c = DAT_0071d174;
  uStack_320 = 0x4d884c;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_320 = 0;
  uStack_324 = 6;
  uStack_328 = 0;
  piStack_32c = DAT_0071d174;
  acStack_330[0] = '`';
  acStack_330[1] = -0x78;
  acStack_330[2] = 'M';
  acStack_330[3] = '\0';
  (**(code **)(*DAT_0071d174 + 0x10c))();
  acStack_330[0] = '\x01';
  acStack_330[1] = '\0';
  acStack_330[2] = '\0';
  acStack_330[3] = '\0';
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
  (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,0,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9c,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9d,0);
  (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,3,0x13f,unaff_ESI + 0x5d8,0x18);
  (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,3,4,unaff_ESI + 0x44,0x18);
  uVar1 = 0x3f800000;
  uVar2 = 0x3f800000;
  uVar3 = 0x3f800000;
  uVar4 = 0x3f800000;
  FUN_004944c0(1,0xffffffff,0,0,5,0);
  DAT_006e4744 = uVar4;
  DAT_006e4740 = uVar3;
  DAT_006e473c = uVar2;
  DAT_006e4738 = uVar1;
  DAT_006e4748 = 0;
  _sprintf(acStack_330,"%s|n%.2f bps sent|n%.2f bps recv",unaff_ESI + 0x23e0,
           (double)*(float *)(unaff_ESI + 200),(double)*(float *)(unaff_ESI + 0xcc));
  chimera__draw_8_bit_text(0,0,acStack_330);
  _sprintf(acStack_330,"%d",*(undefined4 *)(unaff_ESI + 0x23d8));
  chimera__draw_8_bit_text(0,0,acStack_330);
  __ftol();
  _sprintf(acStack_330,"%d");
  chimera__draw_8_bit_text(0,0,acStack_330);
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return;
}
#endif
```
