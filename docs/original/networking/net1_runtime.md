# Original notes: networking `net1_runtime`

Author notes and decompile blocks moved verbatim from the C files converted into the net1_runtime sources.

## network_connection_stats_end.c

```
// network_connection_stats_end  (Ghidra: network_connection_stats_end, already named)
// address 0x440d20, size 82 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/networking_types_notes.md "network_connection_statistics (0x44)":
// "network_connection_stats_end (0x440d20) does +0x00 += now - +0x04 and clears +0x04 and the
// +0x08 byte, which names the first three fields."
// register convention: __cdecl per Ghidra's own signature, but the body calls
// network_connection_stats_lookup_or_add with no arguments of its own -- this function never
// touches EBX/DI itself, so it is simply passing its caller's connection id (EBX) and
// connection key (DI) straight through untouched. Declared here as ordinary parameters so
// that pass-through is explicit; see network_connection_stats_lookup_or_add.c for the same
// register convention on the callee side.
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)
```

```
#if 0
Original Ghidra decompilation (0x440d20):

void __cdecl network_connection_stats_end(void)

{
  int iVar1;
  int iVar2;

  if (((2 < DAT_0087ac06) && (iVar1 = network_connection_stats_lookup_or_add(), iVar1 != -1)) &&
     ((&DAT_0087bec8)[iVar1 * 0x44] != '\0')) {
    iVar2 = FUN_00449210();
    (&DAT_0087bec0)[iVar1 * 0x11] =
         (&DAT_0087bec0)[iVar1 * 0x11] + (iVar2 - (&DAT_0087bec4)[iVar1 * 0x11]);
    (&DAT_0087bec4)[iVar1 * 0x11] = 0;
    (&DAT_0087bec8)[iVar1 * 0x44] = 0;
  }
  return;
}
#endif
```

## network_connection_stats_log_tick.c

```
// network_connection_stats_log_tick  (Ghidra: network_connection_stats_log_tick, already named)
// address 0x440d80, size 660 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/networking_types_notes.md "network_connection_statistics (0x44)": the
// header names interval_packets_sent/interval_bytes_sent/interval_reliable_bytes_sent/
// interval_resend_bytes_sent directly from this function's own log header and %d group, and
// the player-count accumulation ("network_client + 0xb14 or network_server + 8", reading
// +0x1a0) is the cross-check that pinned network_game_session::player_count.
// register convention: __cdecl, no arguments.
//
// The stack copy of "Gamespy Metrics" (0x65fd4c) is NOT dead: it is the requested_path handed to
// network_log_path_resolve (ESI), whose shared buffer becomes the log directory. The manual
// find-end-of-string / dword-copy loops are ordinary strcpy/strcat; the mode string at
// 0x0065fd30 is "wt".
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)

// VERIFIED against disassembly 0x440d80..0x441014 (2026-09-30): FIXED: the log directory comes from network_log_path_resolve("Gamespy Metrics") (0x4e40a0, ESI = the stack copy of that text), not join_game_server_browser_tick; formats, columns, offsets, /1000, 100 ms gate and cleared fields all compared
```

```
#if 0
Original Ghidra decompilation (0x440d80):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl network_connection_stats_log_tick(void)

{
  char cVar1;
  undefined4 *puVar2;
  int iVar3;
  tm *_Tm;
  char *pcVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  char *pcVar8;
  int *piVar9;
  undefined4 *puVar10;
  bool bVar11;
  undefined8 local_21c;
  undefined4 local_214;
  undefined4 local_210;
  undefined1 local_20c [4];
  char local_208 [4];
  undefined1 local_204 [256];
  char local_104 [260];

  if ((2 < DAT_0087ac06) && (DAT_006f14b4 == '\x01')) {
    iVar3 = FUN_00449210();
    if (DAT_006869bd == '\x01') {
      local_214 = 0x20797073;
      local_21c._4_4_ = 0x656d6147;
      local_210 = 0x7274654d;
      local_20c = (undefined1  [4])&DAT_00736369;
      DAT_006869bd = '\0';
      DAT_006a4038 = iVar3;
      DAT_006a8148 = iVar3;
      FID_conflict___time32((__time32_t *)&local_21c);
      _Tm = _localtime(&local_21c);
      _strftime(local_104,0x103,"%Y-%m-%d %H_%M_%S",_Tm);
      pcVar4 = (char *)FUN_004e40a0();
      pcVar8 = local_208;
      do {
        cVar1 = *pcVar4;
        pcVar4 = pcVar4 + 1;
        *pcVar8 = cVar1;
        pcVar8 = pcVar8 + 1;
      } while (cVar1 != '\0');
      directory_create_recursive(local_208);
      pcVar8 = local_20c + 3;
      do {
        pcVar4 = pcVar8;
        pcVar8 = pcVar4 + 1;
      } while (pcVar4[1] != '\0');
      builtin_strncpy(pcVar4 + 1,"\\gamespy ",10);
      pcVar8 = local_104;
      do {
        cVar1 = *pcVar8;
        pcVar8 = pcVar8 + 1;
      } while (cVar1 != '\0');
      uVar5 = (int)pcVar8 - (int)local_104;
      pcVar8 = local_20c + 3;
      do {
        pcVar4 = pcVar8 + 1;
        pcVar8 = pcVar8 + 1;
      } while (*pcVar4 != '\0');
      pcVar4 = local_104;
      for (uVar7 = uVar5 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
        *(undefined4 *)pcVar8 = *(undefined4 *)pcVar4;
        pcVar4 = pcVar4 + 4;
        pcVar8 = pcVar8 + 4;
      }
      for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *pcVar8 = *pcVar4;
        pcVar4 = pcVar4 + 1;
        pcVar8 = pcVar8 + 1;
      }
      puVar2 = (undefined4 *)(local_20c + 3);
      do {
        puVar10 = puVar2;
        puVar2 = (undefined4 *)((int)puVar10 + 1);
      } while (*(char *)((int)puVar10 + 1) != '\0');
      *(undefined4 *)((int)puVar10 + 1) = 0x736c782e;
      *(undefined1 *)((int)puVar10 + 5) = 0;
      DAT_006f14b8 = (FILE *)FUN_00624186(local_208,&DAT_0065fd30);
      _fprintf(DAT_006f14b8,
               "\tEach connection has five columns (see headers below). One empty column separates each connection. Note: resend traffic is considered unreliable.\n"
              );
      _fprintf(DAT_006f14b8,
               "Time\tPackets Sent\tTotal Sent\tReliable Sent\tUnreliable Sent\tResends Sent\n");
    }
    if (100 < (uint)(iVar3 - DAT_006a4038)) {
      DAT_006a4038 = iVar3;
      _fprintf(DAT_006f14b8,"%d",(uint)(iVar3 - DAT_006a8148) / 1000);
      if (0 < DAT_006f14bc) {
        _fprintf(DAT_006f14b8,"\t");
        iVar3 = 0;
        if (0 < DAT_006f14bc) {
          piVar9 = &DAT_0087bee4;
          do {
            _fprintf(DAT_006f14b8,"%d\t%d\t%d\t%d\t%d\t%c",piVar9[6],*piVar9,piVar9[2],
                     *piVar9 - piVar9[2],piVar9[3],-(uint)(DAT_006f14bc + -1 != iVar3) & 9);
            iVar6 = DAT_0071c2d8;
            bVar11 = DAT_0071c2d8 == 0;
            piVar9[6] = 0;
            *piVar9 = 0;
            piVar9[2] = 0;
            piVar9[3] = 0;
            piVar9[1] = 0;
            if (bVar11) {
              if (DAT_0071c2d4 != 0) {
                iVar6 = DAT_0071c2d4 + 8;
                goto LAB_00440fcd;
              }
            }
            else {
              iVar6 = iVar6 + 0xb14;
LAB_00440fcd:
              if (iVar6 != 0) {
                _DAT_0087beb8 = _DAT_0087beb8 + 1;
                _DAT_0087beb4 = _DAT_0087beb4 + *(short *)(iVar6 + 0x1a0);
              }
            }
            iVar3 = iVar3 + 1;
            piVar9 = piVar9 + 0x11;
          } while (iVar3 < DAT_006f14bc);
        }
      }
      _fprintf(DAT_006f14b8,"\n");
    }
  }
  return;
}
#endif
```

## network_connection_stats_lookup_or_add.c

```
// network_connection_stats_lookup_or_add  (Ghidra: network_connection_stats_lookup_or_add,
// already named)
// address 0x440a80, size 160 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: out/phase4/networking_types_notes.md "network_connection_statistics (0x44) and
// network_summary_statistics (0x1c)": the stride is pinned three ways in this exact function
// ("(&DAT_0087becc)[i*0x11]" dword array, "(&DAT_0087bed0)[i*0x22]" word array and
// "(&DAT_0087bec8)[i*0x44]" byte array all resolve to the same element").
// register convention: connection id in EBX (unaff_EBX), connection key in DI, the low word
// of EDI (unaff_DI). Ghidra types this void; callers (network_connection_stats_record_packet, network_connection_stats_end)
// both use its result as an index, so the found/new slot index is returned via the implicit
// EAX convention (same situation as src/memory/circular_buffer_new.c).
// UNSURE: once network_connection_stats_count reaches k_network_connection_stats_count (255)
// it stops growing, but a new record's index is still taken from the un-clamped count value,
// so a 256th distinct connection writes one record past the end of the array. Preserved
// exactly as decompiled; not a rewrite bug.
```

```
#if 0
Original Ghidra decompilation (0x440a80):

void network_connection_stats_lookup_or_add(void)

{
  int iVar1;
  int iVar2;
  short *psVar3;
  int unaff_EBX;
  short unaff_DI;

  iVar1 = DAT_006f14bc;
  iVar2 = 0;
  if (0 < DAT_006f14bc) {
    psVar3 = &DAT_0087bed0;
    do {
      if ((*(int *)(psVar3 + -2) == unaff_EBX) && (*psVar3 == unaff_DI)) {
        if (iVar2 != -1) {
          return;
        }
        break;
      }
      iVar2 = iVar2 + 1;
      psVar3 = psVar3 + 0x22;
    } while (iVar2 < DAT_006f14bc);
  }
  if (DAT_006f14bc < 0xff) {
    DAT_006f14bc = DAT_006f14bc + 1;
  }
  iVar2 = iVar1 * 0x44;
  (&DAT_0087becc)[iVar1 * 0x11] = unaff_EBX;
  (&DAT_0087bed0)[iVar1 * 0x22] = unaff_DI;
  (&DAT_0087bec0)[iVar1 * 0x11] = 0;
  (&DAT_0087bed4)[iVar1 * 0x11] = 0;
  (&DAT_0087bed8)[iVar1 * 0x11] = 0;
  *(undefined4 *)(&DAT_0087bedc + iVar2) = 0;
  *(undefined4 *)(&DAT_0087bee0 + iVar2) = 0;
  (&DAT_0087bee4)[iVar1 * 0x11] = 0;
  (&DAT_0087bee8)[iVar1 * 0x11] = 0;
  (&DAT_0087beec)[iVar1 * 0x11] = 0;
  (&DAT_0087bef0)[iVar1 * 0x11] = 0;
  *(undefined4 *)(&DAT_0087bef4 + iVar2) = 0;
  *(undefined4 *)(&DAT_0087bef8 + iVar2) = 0;
  (&DAT_0087befc)[iVar1 * 0x11] = 0;
  *(undefined4 *)(&DAT_0087bf00 + iVar2) = 0;
  return;
}
#endif
```

## network_connection_stats_record_packet.c

```
// network_connection_stats_record_packet  (Ghidra: FUN_00440b20, still unnamed -> renamed)
// address 0x440b20, size 505 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("updates a connection's per-packet
// statistics counters ... for one transmitted or received packet"); out/phase4/
// networking_types_notes.md "network_connection_statistics (0x44)" names every field this
// function touches; the "misattributed" note explains that 0x440b20 (with 0x440d80) is what
// let the session offsets (network_client + 0xb14, network_server + 8) be cross-checked, and
// that the GameSpy connection object this function keys off of is foreign library data, not a
// Blam struct.
// register convention: GameSpy connection handle in EAX (in_EAX), payload byte length in ECX
// (in_ECX), then three Ghidra-recognized stack byte flags: is_sent, is_reliable, is_resend.
// FIXED in the review pass: gamespy_array_length and gt2GetRemotePort are one-instruction GameSpy
// accessors (0x6175f0 returns the uint32 at object+0x00; 0x6147d0 returns the uint16 at
// object+0x04 in AX), and the disassembly at 0x440b8a..0x440b9b is
//   push edi / call 0x6175f0 / push edi / mov ebx,eax / call 0x6147d0 / movzx edi,ax
// followed by the call to 0x440a80, so they supply exactly the connection id and key
// network_connection_stats_lookup_or_add needs, both read off the GameSpy connection and
// the key zero-extended from 16 bits.
// UNSURE: `0x1c` added to the payload length is assumed to be a fixed per-packet header/
// overhead byte count (e.g. IP+UDP+protocol headers), not independently confirmed.
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)
```

```
#if 0
Original Ghidra decompilation (0x440b20):

void FUN_00440b20(char param_1,char param_2,char param_3)

{
  int in_EAX;
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int in_ECX;
  int iVar4;

  if (2 < DAT_0087ac06) {
    iVar4 = in_ECX + 0x1c;
    if (param_1 == '\0') {
      FUN_004d7a50(1);
    }
    else {
      FUN_004d79d0(1);
    }
    if (((DAT_006f14b4 == '\x01') && (in_EAX != 0)) && (iVar1 = FUN_00614840(), iVar1 != 0)) {
      if (*(int *)(iVar1 + 0x14) == -1) {
        FUN_006175f0();
        FUN_006147d0();
        iVar2 = network_connection_stats_lookup_or_add();
        *(int *)(iVar1 + 0x14) = iVar2;
        (&DAT_0087bec8)[iVar2 * 0x44] = 1;
        uVar3 = FUN_00449210();
        (&DAT_0087bec4)[*(int *)(iVar1 + 0x14) * 0x11] = uVar3;
      }
      iVar2 = *(int *)(iVar1 + 0x14);
      if (param_1 == '\x01') {
        (&DAT_0087bed4)[iVar2 * 0x11] = (&DAT_0087bed4)[iVar2 * 0x11] + iVar4;
        (&DAT_0087bee4)[*(int *)(iVar1 + 0x14) * 0x11] =
             (&DAT_0087bee4)[*(int *)(iVar1 + 0x14) * 0x11] + iVar4;
        iVar2 = *(int *)(iVar1 + 0x14) * 0x44;
        *(int *)(&DAT_0087bef4 + iVar2) = *(int *)(&DAT_0087bef4 + iVar2) + 1;
        (&DAT_0087befc)[*(int *)(iVar1 + 0x14) * 0x11] =
             (&DAT_0087befc)[*(int *)(iVar1 + 0x14) * 0x11] + 1;
        if (param_2 == '\x01') {
          iVar2 = *(int *)(iVar1 + 0x14) * 0x44;
          *(int *)(&DAT_0087bedc + iVar2) = *(int *)(&DAT_0087bedc + iVar2) + iVar4;
          (&DAT_0087beec)[*(int *)(iVar1 + 0x14) * 0x11] =
               (&DAT_0087beec)[*(int *)(iVar1 + 0x14) * 0x11] + iVar4;
        }
        if (param_3 == '\x01') {
          iVar2 = *(int *)(iVar1 + 0x14) * 0x44;
          *(int *)(&DAT_0087bee0 + iVar2) = *(int *)(&DAT_0087bee0 + iVar2) + iVar4;
          (&DAT_0087bef0)[*(int *)(iVar1 + 0x14) * 0x11] =
               (&DAT_0087bef0)[*(int *)(iVar1 + 0x14) * 0x11] + iVar4;
        }
        DAT_0087beac = DAT_0087beac + 1;
        DAT_0087bea4 = DAT_0087bea4 + iVar4;
        return;
      }
      (&DAT_0087bed8)[iVar2 * 0x11] = (&DAT_0087bed8)[iVar2 * 0x11] + iVar4;
      (&DAT_0087bee8)[*(int *)(iVar1 + 0x14) * 0x11] =
           (&DAT_0087bee8)[*(int *)(iVar1 + 0x14) * 0x11] + iVar4;
      iVar2 = *(int *)(iVar1 + 0x14) * 0x44;
      *(int *)(&DAT_0087bef8 + iVar2) = *(int *)(&DAT_0087bef8 + iVar2) + 1;
      iVar1 = *(int *)(iVar1 + 0x14) * 0x44;
      *(int *)(&DAT_0087bf00 + iVar1) = *(int *)(&DAT_0087bf00 + iVar1) + 1;
      DAT_0087beb0 = DAT_0087beb0 + 1;
      DAT_0087bea8 = DAT_0087bea8 + iVar4;
    }
  }
  return;
}
#endif
```

## network_debug_fill_canary_buffer.c

```
// network_debug_fill_canary_buffer  (Ghidra: FUN_004e0790, unnamed)
// address 0x4e0790, size 117 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Writes a fixed, human-readable
// filler/canary byte pattern ('message in a bot...') into the 20-byte buffer pointed to by
// in_EAX, likely a debug placeholder value." The four dwords decode (little-endian) to the
// ASCII text "message in a bot".
// register convention: EAX = buffer (uint32_t[5]).
// blam-cc: EAX -> buffer

// VERIFIED against disassembly 0x4e0790..0x4e0805 (2026-09-30): four dwords = "message in a bot"; 5th dword never written
```

```
#if 0
Original Ghidra decompilation (0x4e0790):

void FUN_004e0790(void)

{
  undefined4 *in_EAX;

  *in_EAX = 0;
  *in_EAX = 0x7373656d;
  in_EAX[1] = 0x20656761;
  in_EAX[2] = 0x61206e69;
  in_EAX[3] = 0x746f6220;
  return;
}
#endif
```

## network_dispatch_initialize.c

```
// network_dispatch_initialize  (Ghidra: FUN_004414c0, still unnamed -> renamed)
// address 0x4414c0, size 72 bytes
// name confidence: 0.45   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("initializes networking (winsock etc.)
// and, on first run, registers the network game-message dispatch group used to route incoming
// gameplay packets"); types/networking.h's closing note that 0x006994f8 is the
// data_packet_group 0x4414c0 registers (39 types, max decoded size 0x600), reusing
// src/memory/struct_definition_table_compute_sizes.c's already-established prototype.
// register convention: __cdecl, no arguments.
// UNSURE: DAT_00718fa4, DAT_0071973c and the byte-sliced object at DAT_00719754 are not
// documented anywhere in networking_types_notes.md; network_initialize (0x4415c0, also in
// this module) shares DAT_007196ec with this function as a gate, so it is named here as a
// plausible "networking disabled" flag, but that is not independently confirmed either.
```

```
#if 0
Original Ghidra decompilation (0x4414c0):

void FUN_004414c0(void)

{
  short sVar1;

  sVar1 = network_initialize();
  if (sVar1 != 0) {
    if (DAT_00718fa4 == -1) {
      DAT_00718fa4 = 5;
    }
    DAT_00719754._0_2_ = 0xffff;
    DAT_0071973c = 0;
    DAT_00719754._3_1_ = 1;
  }
  if (DAT_007196ec == 0) {
    struct_definition_table_compute_sizes(&PTR_s_network_game_messages_group_006994f8);
  }
  return;
}
#endif
```

## network_event_feed_flush.c

```
// network_event_feed_flush  (Ghidra: FUN_004e8040; named per this rewrite)
// address 0x4e8040, size 405 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md ("Resolves queued unit-index event records to
// network indices and sends them as a batched message-0x26 update, then clears the queue.");
// network_event_feed_queue_append.c (this batch, 0x4e7ff0, the same queue layout: count at +4,
// keys at +8 stride 8, payloads at +0x88 stride 0x30); types/objects.h hash_table (the manual
// bucket walk here matches hash_table_get's own bucket-chain shape exactly, just inlined).
// register convention: none beyond the one recognized stack parameter (the queue pointer).
// UNSURE: the manual hash-table walk assumes PTR_DAT_00687558 is the same wrapper as
// machine_table (hash_table embedded at +0xc), matching this batch's other
// hash_table_get call sites against that global; not independently re-derived here.
```

```
#if 0
Original Ghidra decompilation (0x4e8040), from tools/pack.py 0x4e8040:

void FUN_004e8040(int *param_1)

{
  int iVar1;
  short *psVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  int iVar6;
  short sVar7;
  int *piVar8;
  int *piVar9;
  void **ppvVar10;
  bool force_changed;
  undefined4 *puVar11;
  int local_c4;
  undefined4 local_c0 [16];
  void *local_80 [16];
  undefined4 local_40 [16];

  puVar11 = local_c0;
  for (iVar6 = 0x10; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar11 = 0;
    puVar11 = puVar11 + 1;
  }
  ppvVar10 = local_80;
  for (iVar6 = 0x10; iVar6 != 0; iVar6 = iVar6 + -1) {
    *ppvVar10 = (void *)0x0;
    ppvVar10 = ppvVar10 + 1;
  }
  iVar6 = param_1[1];
  local_c4 = 0;
  if (0 < iVar6) {
    piVar8 = param_1 + 0x22;
    piVar9 = param_1;
    do {
      piVar9 = piVar9 + 2;
      iVar1 = *piVar9;
      if (((iVar1 != -1) && (sVar5 = (short)iVar1, -1 < sVar5)) &&
         (sVar5 < *(short *)(DAT_0087a480 + 0x20))) {
        psVar2 = (short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar5 +
                          *(int *)(DAT_0087a480 + 0x34));
        sVar5 = *psVar2;
        if ((sVar5 != 0) && ((sVar7 = (short)((uint)iVar1 >> 0x10), sVar7 == 0 || (sVar5 == sVar7)))
           ) {
          local_c0[local_c4] = piVar9;
          local_80[local_c4] = piVar8;
          local_40[local_c4] = psVar2 + 0x98;
          local_c4 = local_c4 + 1;
        }
      }
      piVar8 = piVar8 + 0xc;
      iVar6 = iVar6 + -1;
    } while (iVar6 != 0);
  }
  iVar6 = 0;
  if (0 < local_c4) {
    do {
      iVar1 = *(int *)local_c0[iVar6];
      iVar3 = 0;
      if (iVar1 != -1) {
        iVar3 = -1;
        if (PTR_DAT_00687558[0xc] == '\x01') {
          iVar4 = iVar1;
          if (iVar1 < 0) {
            iVar4 = -iVar1;
          }
          for (piVar8 = *(int **)(*(int *)(PTR_DAT_00687558 + 0x14) + 4 +
                                 (iVar4 % *(int *)(PTR_DAT_00687558 + 0x10)) * 8);
              piVar8 != (int *)0x0; piVar8 = (int *)piVar8[2]) {
            if (*piVar8 == iVar1) {
              iVar3 = piVar8[1];
              break;
            }
          }
        }
        if (iVar3 == -1) {
          iVar3 = 0;
        }
      }
      *(int *)local_c0[iVar6] = iVar3;
      iVar6 = iVar6 + 1;
    } while (iVar6 < local_c4);
  }
  force_changed = (char)*param_1 != '\x01';
  if (force_changed) {
    puVar11 = local_40;
  }
  else {
    puVar11 = (undefined4 *)0x0;
  }
  message_delta_encode_message
            ((uint)force_changed,0x26,(int)local_c0,local_80,(int)puVar11,local_c4,force_changed);
  FUN_004e1a80(1,&DAT_00871de0,(char)*param_1,0,0,2);
  param_1[1] = 0;
  return;
}
#endif
```

## network_event_feed_queue_append.c

```
// network_event_feed_queue_append  (Ghidra: network_event_feed_queue_append, already named)
// address 0x4e7ff0, size 68 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md ("Queues one event record into a 16-slot buffer
// and triggers a flush of the buffer once it fills."); network_event_feed_flush (this module, 0x4e8040) is
// the flush this calls once the 16th slot is filled.
// register convention: EAX -> queue (the event feed state block), EDX -> key (a 2-dword header:
// a network index/type pair), stack -> payload (a 12-dword record copied into the slot).
//   // blam-cc: EAX -> queue, EDX -> key, stack -> payload
// UNSURE: no header declares this queue's type; it is not one this batch's types notes attribute
// to networking.h (the summary calls it a generic 16-slot buffer), so it is left as a raw byte
// pointer with offsets exactly as Ghidra shows rather than inventing a struct.
```

```
#if 0
Original Ghidra decompilation (0x4e7ff0), from tools/pack.py 0x4e7ff0:

void network_event_feed_queue_append(undefined4 *param_1)

{
  int in_EAX;
  int iVar1;
  undefined4 *in_EDX;
  int iVar2;
  undefined4 *puVar3;

  iVar2 = *(int *)(in_EAX + 4);
  *(undefined4 *)(in_EAX + 8 + iVar2 * 8) = *in_EDX;
  *(undefined4 *)(in_EAX + 0xc + iVar2 * 8) = in_EDX[1];
  puVar3 = (undefined4 *)(iVar2 * 0x30 + 0x88 + in_EAX);
  for (iVar1 = 0xc; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *param_1;
    param_1 = param_1 + 1;
    puVar3 = puVar3 + 1;
  }
  iVar2 = *(int *)(in_EAX + 4) + 1;
  *(int *)(in_EAX + 4) = iVar2;
  if (iVar2 == 0x10) {
    FUN_004e8040();
    return;
  }
  return;
}
#endif
```

## network_hostname_thread_proc.c

```
// network_hostname_thread_proc  (Ghidra: network_hostname_thread_proc, already named)
// address 0x441510, size 34 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/networking_functions.md summary ("thread entry point that retrieves the
// local machine's hostname into a buffer, signals completion, and exits the thread"); Ghidra
// already recovered the full __stdcall signature and both Win32 calls by name.
// register convention: __stdcall, one recognized stack parameter (hostname_buffer).
```

```
#if 0
Original Ghidra decompilation (0x441510):

void network_hostname_thread_proc(char *hostname_buffer)

{
  gethostname(hostname_buffer,0x100);
  DAT_006f14cc = 1;
                    /* WARNING: Subroutine does not return */
  ExitThread(0);
}
#endif
```

## network_index_cache_find_or_allocate_slot.c

```
// network_index_cache_find_or_allocate_slot  (Ghidra: FUN_004e9c20; named per this rewrite)
// address 0x4e9c20, size 161 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md ("Looks up or allocates a free slot in a
// fixed-size hash-indexed table, evicting via a rotating cursor when no existing entry matches.");
// out/phase4/networking_types_notes.md's note on 0x4e9c20/0x4e9cd0/0x4e9d20/0x4e9d40 (a
// hash-indexed object-to-network-index cache; the underlying hash_table, types/objects.h, is
// generic and owned by another module, but the eviction cursor behaviour here is local); the
// sibling functions in this file group confirm the wrapper's hash_table is embedded at +0xc
// (matching object_network_id_table/remote_player_index_remap_table's own +0xc convention).
// register convention: EAX -> container (whose +0x58 holds the cache pointer), stack -> key.
//   // blam-cc: EAX -> container, stack -> key
// UNSURE: hash_table_set_or_remove's real signature; called here with only the evicted slot index
// visible, so EAX/ECX are assumed to carry the table and the new key (dropped by the decompile).
```

```
#if 0
Original Ghidra decompilation (0x4e9c20), from tools/pack.py 0x4e9c20:

int FUN_004e9c20(int param_1)

{
  int *piVar1;
  int *piVar2;
  int in_EAX;
  int iVar3;
  int iVar4;
  int iVar5;

  piVar1 = *(int **)(in_EAX + 0x58);
  iVar4 = -1;
  if (((char)piVar1[3] == '\x01') && (param_1 != -1)) {
    iVar3 = param_1;
    if (param_1 < 0) {
      iVar3 = -param_1;
    }
    for (piVar2 = *(int **)(piVar1[5] + 4 + (iVar3 % piVar1[4]) * 8); piVar2 != (int *)0x0;
        piVar2 = (int *)piVar2[2]) {
      if (*piVar2 == param_1) {
        iVar4 = piVar2[1];
        break;
      }
    }
  }
  if (iVar4 != -1) {
    return iVar4;
  }
  iVar4 = piVar1[9];
  iVar5 = -1;
  iVar3 = iVar4;
  while( true ) {
    if (*(int *)(piVar1[10] + iVar3 * 4) == -1) {
      iVar5 = iVar3;
    }
    piVar1[9] = iVar3 + 1;
    if (*piVar1 <= iVar3 + 1) {
      piVar1[9] = 0;
    }
    iVar3 = piVar1[9];
    if (iVar4 == iVar3) break;
    if (iVar5 != -1) {
LAB_004e9c9d:
      hash_table_set_or_remove(iVar5);
      *(int *)(piVar1[10] + iVar5 * 4) = param_1;
      return iVar5;
    }
  }
  if (iVar5 == -1) {
    return -1;
  }
  goto LAB_004e9c9d;
}
#endif
```

## network_index_cache_get.c

```
// network_index_cache_get  (Ghidra: FUN_004e9d20; named per this rewrite)
// address 0x4e9d20, size 25 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md ("Thin guarded wrapper around hash_table_get that
// treats a -1 key as 'no entry'.").
// register convention: ECX -> key; the hash_table pointer itself is presumably EAX, dropped by
// the decompile since it is passed straight through to hash_table_get unexamined.
//   // blam-cc: ECX -> key
// UNSURE: the table argument (in_EAX in the original, forwarded to hash_table_get) is modeled
// here as an explicit parameter since Ghidra shows zero visible arguments at the call site.
```

```
#if 0
Original Ghidra decompilation (0x4e9d20), from tools/pack.py 0x4e9d20:

undefined4 FUN_004e9d20(void)

{
  undefined4 uVar1;
  int in_ECX;

  uVar1 = 0;
  if (in_ECX != -1) {
    uVar1 = hash_table_get();
  }
  return uVar1;
}
#endif
```

## network_index_cache_insert_if_free.c

```
// network_index_cache_insert_if_free  (Ghidra: FUN_004e9cd0; named per this rewrite)
// address 0x4e9cd0, size 74 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md ("Inserts a value into an index-cache slot only if
// that slot is not already occupied and no existing hash mapping exists."); see
// network_index_cache_find_or_allocate_slot.c (this batch, 0x4e9c20) for the shared
// network_index_cache layout (slots array at +0x28), now declared in types/networking.h.
// register convention: EAX -> container, ECX -> key, stack -> slot.
//   // blam-cc: EAX -> container, ECX -> key, stack -> slot
// FIXED (register inputs, objdump): ECX carries key (read at 0x4e9ced, live across the call to
// hash_table_get, which never writes ecx -- confirmed against its own disassembly). Ghidra's
// extraout_ECX was this same incoming key, not a second value; the old "key_value" stack
// parameter was a phantom -- the function only ever reads one stack slot (slot).
```

```
#if 0
Original Ghidra decompilation (0x4e9cd0), from tools/pack.py 0x4e9cd0:

undefined4 FUN_004e9cd0(int param_1)

{
  int *piVar1;
  int in_EAX;
  int iVar2;
  int extraout_ECX;

  piVar1 = (int *)(*(int *)(*(int *)(in_EAX + 0x58) + 0x28) + param_1 * 4);
  if (*piVar1 != -1) {
    return 0;
  }
  iVar2 = hash_table_get();
  if (iVar2 == -1) {
    *piVar1 = extraout_ECX;
    hash_table_set_or_remove(param_1);
    return 1;
  }
  return 0;
}
#endif
```

## network_index_cache_remove.c

```
// network_index_cache_remove  (Ghidra: FUN_004e9d40; named per this rewrite)
// address 0x4e9d40, size 101 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md ("Removes an entry from the index-cache hash table
// if present, clearing its bound slot."); see network_index_cache_find_or_allocate_slot.c (this
// batch, 0x4e9c20) for the shared network_index_cache layout, now in types/networking.h.
// register convention: EAX -> container, ESI -> key (unaff_ESI in the decompile).
//   // blam-cc: EAX -> container, ESI -> key
// UNSURE: hash_table_set_or_remove's real signature, same caveat as
// network_index_cache_find_or_allocate_slot.c.
```

```
#if 0
Original Ghidra decompilation (0x4e9d40), from tools/pack.py 0x4e9d40:

undefined4 FUN_004e9d40(void)

{
  int iVar1;
  int *piVar2;
  int in_EAX;
  int iVar3;
  int unaff_ESI;

  iVar1 = *(int *)(in_EAX + 0x58);
  if ((*(char *)(iVar1 + 0xc) == '\x01') && (unaff_ESI != -1)) {
    iVar3 = unaff_ESI;
    if (unaff_ESI < 0) {
      iVar3 = -unaff_ESI;
    }
    for (piVar2 = *(int **)(*(int *)(iVar1 + 0x14) + 4 + (iVar3 % *(int *)(iVar1 + 0x10)) * 8);
        piVar2 != (int *)0x0; piVar2 = (int *)piVar2[2]) {
      if (*piVar2 == unaff_ESI) {
        if (piVar2[1] == -1) {
          return 0;
        }
        *(undefined4 *)(*(int *)(iVar1 + 0x28) + piVar2[1] * 4) = 0xffffffff;
        hash_table_set_or_remove(0xffffffff);
        return 1;
      }
    }
  }
  return 0;
}
#endif
```

## network_initialize.c

```
// network_initialize  (Ghidra: network_initialize, already named)
// address 0x4415c0, size 281 bytes
// name confidence: 0.55   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("one-time networking subsystem
// startup: calls WSAStartup, determines the local IP address, and launches the background
// network processing thread"); shares network_disabled_flag (0x007196ec) and
// network_local_address (0x006869b0, already documented in networking_types_notes.md as
// "byte swapped before binding") with network_dispatch_initialize.c / network_channels_open.c.
// register convention: __cdecl, no arguments.
// UNSURE: DAT_006869b4, DAT_006869b8 and DAT_006f14c0 are not documented anywhere in
// networking_types_notes.md; named here from how they are used (resolved/override address,
// one-time-init flag, and an init timestamp respectively) but not independently confirmed.
// UNSURE: WSADATA is a Winsock structure with no Blam equivalent; the manual zero-fill loop
// is folded into an equivalent memset over a same-size opaque byte buffer (same technique as
// the strcpy/strcat foldings in the summary/connection stats log functions), since the loop's
// only effect is zeroing the whole 400-byte structure before WSAStartup fills it in.
// UNSURE: the hostent parsing (`**(hostent+0xc)`, i.e. h_addr_list[0] dereferenced) is a raw
// Winsock structure walk with no Blam type, same treatment as network_local_hostent_get.c.
```

```
#if 0
Original Ghidra decompilation (0x4415c0):

short network_initialize(void)

{
  int iVar1;
  int iVar2;
  short sVar3;
  WORD *pWVar4;
  void *local_1ac [2];
  uint local_1a4;
  WSADATA local_198;

  sVar3 = 0;
  local_1ac[0] = (void *)0x0;
  if (DAT_007196ec != 0) {
    DAT_006869b8 = 0;
    return 0;
  }
  if (DAT_006869b8 == '\0') {
    local_198.wVersion = 0;
    pWVar4 = &local_198.wHighVersion;
    for (iVar2 = 99; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined4 *)pWVar4 = 0;
      pWVar4 = pWVar4 + 2;
    }
    *pWVar4 = 0;
    iVar2 = WSAStartup(2,&local_198);
    if ((short)iVar2 == 0) {
      if (DAT_006869b0 == 0) {
        iVar1 = network_local_hostent_get(local_1ac);
        if (iVar1 == 0) {
          return -0x10;
        }
        local_1a4 = *(uint *)**(undefined4 **)((int)local_1ac[0] + 0xc);
        DAT_006869b4 = (local_1a4 & 0xff0000 | local_1a4 >> 0x10) >> 8 |
                       (local_1a4 << 0x10 | local_1a4 & 0xff00) << 8;
      }
      else {
        DAT_006869b4 = DAT_006869b0;
      }
    }
    CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x10400,autopatch_proxy_initialize,(LPVOID)0x0,0,
                 (LPDWORD)local_1ac);
    DAT_006f14c0 = FUN_00449210();
    DAT_006869b8 = '\x01';
    sVar3 = (short)iVar2;
  }
  return sVar3;
}
#endif
```

## network_local_hostent_get.c

```
// network_local_hostent_get  (Ghidra: network_local_hostent_get, already named)
// address 0x441540, size 122 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md summary ("resolves and returns the local
// machine's hostent structure, retrieving the hostname on a watchdog-timed worker thread
// first"); Ghidra already recovered the full signature. Shares network_hostname_ready
// (0x006f14cc) with network_hostname_thread_proc.c and the hostname buffer with it too.
// register convention: __cdecl, one recognized parameter (out_hostent).
// UNSURE: `hostent` itself is a Winsock structure, not a Blam type; kept as an opaque
// void * returned straight from gethostbyname, same treatment as
// src/networking/network_join_hostname_resolved_callback.c.
```

```
#if 0
Original Ghidra decompilation (0x441540):

int __cdecl network_local_hostent_get(void **out_hostent)

{
  HANDLE hHandle;
  DWORD DVar1;
  hostent *phVar2;
  DWORD local_4;

  DAT_006f14cc = 0;
  hHandle = CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x10400,network_hostname_thread_proc,
                         &DAT_006a4040,0,&local_4);
  if (hHandle != (HANDLE)0x0) {
    DVar1 = WaitForSingleObject(hHandle,10000);
    if (DVar1 == 0x102) {
      TerminateThread(hHandle,0);
    }
    CloseHandle(hHandle);
    if (DAT_006f14cc != 0) {
      phVar2 = gethostbyname(&DAT_006a4040);
      *out_hostent = phVar2;
      return 1;
    }
  }
  return 0;
}
#endif
```

## network_log_path_resolve.c

```
// network_log_path_resolve  (Ghidra: FUN_004e40a0; renamed -- see evidence)
// address 0x4e40a0, size 79 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md's own summary ("builds a default server/profile
// name string") does not match the code: the only work here is zeroing a static 0x104 buffer,
// asking security_check_write_access() whether the process can write to the requested location,
// and formatting a caller-supplied string into the buffer with a plain "%s" (twice, once gated
// on the access check and once unconditionally if the buffer is still empty -- both calls format
// the identical source string, so the second is only ever reached when the first branch was
// skipped by the access check). Its only two call sites (this batch's network_banlist_save at
// 0x4e3380 and a foreign network_stats_summary_log_open at 0x440670, see
// src/networking/network_stats_summary_log_open.c) both use its result immediately as an fopen
// path, which is what the name reflects.
// register convention: no formal parameters recognized by Ghidra, but network_banlist_save's own
// disassembly (`mov esi,0x71c308` immediately before `call 0x4e40a0`, and this function's body
// doing `push esi` as the "%s" vararg) proves the caller-supplied string arrives in ESI.
//   // blam-cc: ESI -> requested_path
// UNSURE: the two snprintf calls format the exact same ESI value in both branches; the "falling
// back to a second source" a low-confidence summary suggested is not what the retail binary
// does, so this rewrite keeps the literal double check-and-retry shape without inventing a
// second source. UNSURE: the exact text of the fopen mode string at 0x0065fd30, reused here by
// this function's one caller in this batch (not captured by string extraction; assumed to be a
// plain text mode such as "wt"). UNSURE: network_stats_summary_log_open.c's own extern for this
// function was written before ESI's role was known and declares it as taking no arguments; that
// file is outside this batch and is not corrected here.
```

```
#if 0
Original Ghidra decompilation (0x4e40a0), from tools/pack.py 0x4e40a0:

undefined1 * FUN_004e40a0(void)

{
  int iVar1;

  DAT_006b85b8 = '\0';
  iVar1 = security_check_write_access();
  if (iVar1 != 0) {
    __snprintf(&DAT_006b85b8,0x104,"%s");
  }
  if (DAT_006b85b8 == '\0') {
    __snprintf(&DAT_006b85b8,0x104,"%s");
  }
  return &DAT_006b85b8;
}

Disassembly (objdump -d -M intel, bin/halo.exe) confirms the ESI-carried vararg:
  4e40a0: mov byte ptr [0x6b85b8],0
  4e40a7: call security_check_write_access
  4e40ac: test eax,eax
  4e40ae: je 0x4e40c8
  4e40b0: push esi
  4e40b1: push 0x65efec
  4e40b6: push 0x104
  4e40bb: push 0x6b85b8
  4e40c0: call __snprintf
  4e40c8: mov al,[0x6b85b8]
  4e40cd: test al,al
  4e40cf: jne 0x4e40e9
  4e40d1: push esi
  4e40d2: push 0x65efec
  4e40d7: push 0x104
  4e40dc: push 0x6b85b8
  4e40e1: call __snprintf
  4e40e9: mov eax,0x6b85b8
  4e40ee: ret
And the caller (network_banlist_save, 0x4e3380):
  4e3386: push 0x65fd30      ; fopen mode, staged early for the later fopen call
  4e338b: mov esi,0x71c308   ; ESI = &network_banlist_full_path, this function's real argument
  4e3390: call 0x4e40a0
  4e3395: push eax
  4e3396: call 0x624186      ; fopen(path=eax, mode=0x65fd30)
#endif
```

## network_message_block_build.c

```
// network_message_block_build  (Ghidra: FUN_00440350, still unnamed -> renamed)
// address 0x440350, size 86 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("allocates or reuses a small heap
// buffer, encodes a length+flags header in its first word, and copies the supplied data
// after the header"); out/phase2/networking/00.md and `python tools/pack.py 0x440350`.
// All seven callers live outside this module (around the message-delta / test-message code
// near 0x4e93xx) and always immediately reassign their own variable from the call result,
// e.g. `local_608 = (ushort *)FUN_00440350(local_608);`, which is why the buffer pointer is
// treated here as an implicit EAX return despite Ghidra's void prototype (same situation as
// src/memory/circular_buffer_new.c).
// register convention: existing buffer pointer or NULL in EAX, source data pointer in ECX,
// flags (2 bits used) in DL (low byte of EDX), byte length on the stack (param_1).
// FIXED (register inputs, objdump): EDX/DL (read at 0x44035c, "mov bl,dl") carries flags; the
// blam-cc wording ("flag bits in DL") didn't match the flags parameter name, so it wasn't
// recognized as claimed. Reworded only; flags was already wired up correctly.
// UNSURE: the header encoding `((flags & 3) | (length + 2) * 4) << 2` is preserved verbatim;
// its consumer (some other module's bit-packed record format) is not recovered here.
// UNSURE: when buffer is already non-NULL its capacity is never checked against length, so a
// caller must already guarantee it is large enough.
```

```
#if 0
Original Ghidra decompilation (0x440350):

void FUN_00440350(uint param_1)

{
  undefined2 *in_EAX;
  undefined4 *in_ECX;
  uint uVar1;
  byte in_DL;
  undefined4 *puVar2;

  if ((in_EAX == (undefined2 *)0x0) &&
     (in_EAX = GlobalAlloc(0,(int)(short)(param_1 + 2)), in_EAX == (undefined2 *)0x0)) {
    return;
  }
  *in_EAX = (short)(((uint)(in_DL & 3) | (param_1 + 2) * 4) << 2);
  if (in_ECX != (undefined4 *)0x0) {
    puVar2 = (undefined4 *)(in_EAX + 1);
    for (uVar1 = (param_1 & 0xffff) >> 2; uVar1 != 0; uVar1 = uVar1 - 1) {
      *puVar2 = *in_ECX;
      in_ECX = in_ECX + 1;
      puVar2 = puVar2 + 1;
    }
    for (param_1 = param_1 & 3; param_1 != 0; param_1 = param_1 - 1) {
      *(undefined1 *)puVar2 = *(undefined1 *)in_ECX;
      in_ECX = (undefined4 *)((int)in_ECX + 1);
      puVar2 = (undefined4 *)((int)puVar2 + 1);
    }
  }
  return;
}
#endif
```

## network_message_read_sized_buffer.c

```
// network_message_read_sized_buffer  (Ghidra: FUN_004de420; named per this rewrite)
// address 0x4de420, size 68 bytes
// name confidence: 0.25   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Register-based helper (EDI) that validates a
// size/capacity value obtained twice from FUN_004cf950 against param_1 before returning the
// buffer pointer, otherwise returns null." Reads a 16-bit chunked header into `buffer` itself,
// checks its high 12 bits (a byte count) against the caller's capacity, then reads that many
// more bits and confirms the bit count consumed matches exactly, before returning `buffer`.
// The 16-bit header carries the total byte count in its high 12 bits; the payload (total*8 - 16
// bits) is read into buffer + 2 bytes, after the header.
// register convention: EDI -> buffer, EBX -> stream. blam-cc: EDI -> buffer, EBX -> stream, stack -> capacity
// FIXED (register inputs, objdump): EBX (read at 0x4de420, "push ebx" as the very first
// instruction, before eax/ecx are even set up) is the stream pointer -- it is pushed as the
// third stack argument to both bit_stream_read_bits_chunked calls. `stream` was already a
// parameter of this rewrite but was wrongly modelled as a stack argument instead of EBX.

// VERIFIED against disassembly 0x4de420..0x4de464 (2026-09-30): FIXED: second read targets buffer + 2 bytes (lea ecx,[edi+2] at 0x4de44b)
```

```
#if 0
Original Ghidra decompilation (0x4de420):

ushort * FUN_004de420(int param_1)

{
  ushort uVar1;
  int iVar2;
  ushort *unaff_EDI;

  iVar2 = bit_stream_read_bits_chunked();
  if ((iVar2 == 0x10) && (uVar1 = *unaff_EDI, (int)(uint)(uVar1 >> 4) <= param_1)) {
    iVar2 = bit_stream_read_bits_chunked();
    if (iVar2 == (uint)(uVar1 >> 4) * 8 + -0x10) {
      return unaff_EDI;
    }
  }
  return (ushort *)0x0;
}
#endif
```

## network_name_string_is_valid_for_mode.c

```
// network_name_string_is_valid_for_mode  (Ghidra: FUN_004e4350; renamed -- see evidence)
// address 0x4e4350, size 276 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md ("Validates that a user-entered string (e.g. a
// player or game name) contains only characters renderable by the UI font and satisfies
// mode-specific leading-character restrictions"); the "ui\\small_ui" tag lookup and
// text_get_character_metrics/FUN_004a8b80 callee pair, both foreign (interface/text modules).
// register convention: __cdecl, three recognized parameters.
// UNSURE: `param_2` (passed to FUN_004a8b80 as an implicit register per that function's own
// unresolved shape) and the exact mode values beyond 1 and 3 are not otherwise attested in this
// batch; FUN_004a8b10/FUN_004a8b80/text_get_character_metrics/tag_lookup are all foreign to this
// module and declared with minimal shapes.
```

```
#if 0
Original Ghidra decompilation (0x4e4350), from tools/pack.py 0x4e4350:

bool FUN_004e4350(char *param_1,undefined4 param_2,int param_3)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  int iVar4;
  bool local_9;

  local_9 = true;
  tag_lookup("ui\\small_ui");
  pcVar2 = param_1;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  if ((param_3 == 3) && (local_9 = *param_1 != '\0', !local_9)) {
    return local_9;
  }
  iVar4 = 0;
  if (0 < (int)pcVar2 - (int)(param_1 + 1)) {
    do {
      if ((((param_1[iVar4] < ' ') || (param_1[iVar4] == 0xff)) ||
          (iVar3 = text_get_character_metrics(), iVar3 == 0)) ||
         (cVar1 = FUN_004a8b80(), cVar1 == '\0')) {
        local_9 = false;
        break;
      }
      if (param_3 == 3) {
        if (iVar4 == 0) {
          if (*param_1 == ' ') {
            return false;
          }
          if (*param_1 == -0x60) {
            return false;
          }
        }
        local_9 = true;
      }
      iVar4 = iVar4 + 1;
    } while (iVar4 < (int)pcVar2 - (int)(param_1 + 1));
  }
  if (param_3 != 1) {
    return local_9;
  }
  if ((local_9 != false) && (cVar1 = FUN_004a8b10(), cVar1 != '\0')) {
    return true;
  }
  return false;
}
#endif
```

## network_password_field_set.c

```
// network_password_field_set  (Ghidra: FUN_004df070; named per this rewrite)
// address 0x4df070, size 27 bytes
// name confidence: 0.25   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md: "Register-based helper that copies a
// wide-character string (e.g. a password) into the object at unaff_ESI+8 and clears the field
// immediately following it." Neither the object nor the exact fields at +8/+0x86 could be tied
// to a specific header-declared struct (network_server_globals.password is 9 wide chars at
// +0x9fc, not +8, and the 0x3f-char copy count does not match its size); kept as raw offsets on
// a generic object pointer.
// register convention: object in ESI (unaff_ESI), source string in EAX (in_EAX). blam-cc:
// EAX -> source, ESI -> object
```

```
#if 0
Original Ghidra decompilation (0x4df070):

uint FUN_004df070(void)

{
  wchar_t *in_EAX;
  wchar_t *pwVar1;
  int unaff_ESI;

  pwVar1 = _wcsncpy((wchar_t *)(unaff_ESI + 8),in_EAX,0x3f);
  *(undefined2 *)(unaff_ESI + 0x86) = 0;
  return (uint)pwVar1 & 0xffffff00;
}
#endif
```

## network_prepare_challenge_packet.c

```
// network_prepare_challenge_packet  (Ghidra: network_prepare_challenge_packet, already named)
// address 0x4deaf0, size 85 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Encodes the queued packet group into a freshly
// allocated 0x600-byte network buffer, matching the chimera-identified 'prepare challenge
// packet' code path."
// FIXED in the review pass. Ghidra shows this as `int network_prepare_challenge_packet(void)`
// with both calls argument-less, and the first draft followed that literally, calling
// network_message_block_build(0x600) -- i.e. it passed the buffer *capacity* where the encoded
// *length* belongs and dropped the destination and flag arguments entirely. The disassembly
// recovers all of it:
//   4deaf0  sub esp,0x604                  ; uint8_t buffer[0x600] plus the capacity dword
//   4deaf7  push 1 / push eax / push ecx / push edx
//   4deb00  lea eax,[esp+0x18]             ; EAX = buffer
//   4deb04  mov ebx,0x6994f8               ; EBX = network_game_messages_group
//   4deb09  mov [esp+0x14],0x600           ; capacity = 0x600
//   4deb11  call 0x4d0ae0                  ; data_packet_group_encode_packet
//   4deb1e  mov eax,[esp]                  ; the encoded byte count written back over capacity
//   4deb21  push eax                       ; -> length
//   4deb22  mov eax,0x6b7f98               ; -> existing block to reuse
//   4deb27  lea ecx,[esp+0x8]              ; -> source = buffer
//   4deb2b  mov dl,3                       ; -> flags = 3
//   4deb2d  call 0x440350                  ; network_message_block_build, result returned in EAX
// So this function takes two register arguments of its own (EAX and EDX), forwards them to the
// encoder, and returns the built message block -- which is exactly how all nine of its callers
// use it (`movzx ecx,WORD PTR [eax]` on the result, e.g. 0x4d9488).
// register convention: message type/class in EAX, payload pointer in EDX.
// blam-cc: EAX -> message_type, EDX -> payload
// UNSURE: data_packet_group_encode_packet's full parameter list is still only known from this
// one call site; the group arrives in EBX and the four stack arguments are given below in the
// order the pushes imply.
```

```
#if 0
Original Ghidra decompilation (0x4deaf0):

int __cdecl network_prepare_challenge_packet(void)

{
  char cVar1;
  int iVar2;

  cVar1 = data_packet_group_encode_packet();
  if (cVar1 != '\0') {
    iVar2 = FUN_00440350(0x600);
    return iVar2;
  }
  return 0;
}
#endif
```

## network_random_offset.c

```
// network_random_offset  (Ghidra: FUN_004403b0, still unnamed -> renamed)
// address 0x4403b0, size 99 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("lazily seeds the C runtime random
// number generator from the current time, then returns a random value offset by a
// caller-supplied base held in ESI"); `python tools/pack.py 0x4403b0`.
// register convention: base offset in ESI (unaff_ESI), no stack arguments.
// UNSURE: `__ftol(iVar2)` is preserved as decompiled, with `rand()`'s int result passed
// straight through it. The real MSVC `__ftol` helper truncates the float already sitting on
// the x87 stack and normally shows with zero visible arguments (see
// src/math/random_seed_generate.c and src/ai/actor_reseed_movement_pause_timer.c for that
// pattern in this codebase); here Ghidra attributed it a visible int argument instead, which
// suggests an intermediate int-to-double-to-int round trip that optimized away. The net
// effect (rand() truncated back to an int) is preserved either way.
```

```
#if 0
Original Ghidra decompilation (0x4403b0):

int FUN_004403b0(void)

{
  __time32_t _Var1;
  int iVar2;
  int unaff_ESI;

  if (DAT_006f0ca8 == '\0') {
    _Var1 = FID_conflict___time32((__time32_t *)0x0);
    FUN_006240c2(_Var1);
    DAT_006f0ca8 = '\x01';
  }
  iVar2 = _rand();
  iVar2 = __ftol(iVar2);
  return iVar2 + unaff_ESI;
}
#endif
```

## network_shutdown.c

```
// network_shutdown  (Ghidra: network_shutdown, already named)
// address 0x4416e0, size 493 bytes
// name confidence: 0.6   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("tears down the networking subsystem:
// closes channels, flushes and closes both statistics log files with final summary lines, and
// clears the initialized flag"); reuses every network_connection_statistics field name from
// out/phase4/networking_types_notes.md, confirming connection_id/connection_key double as an
// IPv4 address and port pair here (they are fed straight into the same address-formatting
// call, gt2AddressToString, that network_channels_open.c uses for socket addresses).
// register convention: __cdecl, no arguments.
// UNSURE: gt2CloseSocket is shown with zero visible arguments at both call sites here, but
// src/networking/network_channels_close.c's decompile of the same callee shows one socket
// argument; the value being tested by the surrounding `if` is passed explicitly here to match.
```

```
#if 0
Original Ghidra decompilation (0x4416e0):

/* WARNING: Removing unreachable block (ram,0x004417e0) */
/* WARNING: Removing unreachable block (ram,0x00441868) */

undefined4 network_shutdown(void)

{
  float fVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  char *pcVar8;
  int local_30;
  undefined1 local_20 [28];

  iVar6 = 0;
  if (DAT_006869b8 != '\0') {
    if (DAT_006f14c8 != 0) {
      FUN_00614860();
      DAT_006f14c8 = 0;
    }
    if (DAT_006f14c4 != 0) {
      FUN_00614860();
      DAT_006f14c4 = 0;
    }
    if (DAT_006a6140 != (FILE *)0x0) {
      _fclose(DAT_006a6140);
    }
    if (DAT_006f14b8 != (FILE *)0x0) {
      _fprintf(DAT_006f14b8,"\n\n");
      iVar7 = 0;
      local_30 = 0;
      iVar5 = 0;
      iVar2 = FUN_00449210();
      uVar4 = (uint)(iVar2 - DAT_006f14c0) / 1000;
      if (0 < DAT_006f14bc) {
        pcVar8 = &DAT_0087bec8;
        do {
          iVar7 = local_30 + *(int *)(pcVar8 + 0xc);
          iVar5 = iVar5 + *(int *)(pcVar8 + 0x10);
          uVar3 = *(uint *)(pcVar8 + -8);
          if (*pcVar8 == '\x01') {
            uVar3 = uVar3 + (iVar2 - *(int *)(pcVar8 + -4));
          }
          FUN_006148b0(*(undefined4 *)(pcVar8 + 4),*(undefined2 *)(pcVar8 + 8),local_20);
          _fprintf(DAT_006f14b8,
                   "Connection [%d]  Live for[%d] seconds  Was address[%s]  Total Sent[%d]  Total Received[%d]  Bytes sent per second[%f]\n"
                   ,iVar6,uVar3 / 1000,local_20,*(undefined4 *)(pcVar8 + 0xc),
                   *(undefined4 *)(pcVar8 + 0x10));
          iVar6 = iVar6 + 1;
          pcVar8 = pcVar8 + 0x44;
          local_30 = iVar7;
        } while (iVar6 < DAT_006f14bc);
      }
      fVar1 = (float)iVar7;
      if (iVar7 < 0) {
        fVar1 = fVar1 + 4.2949673e+09;
      }
      _fprintf(DAT_006f14b8,
               "total data sent[%d]  total received[%d] total time in seconds[%d]  bytes sent per second[%f]\n"
               ,iVar7,iVar5,uVar4,(double)(fVar1 / (float)uVar4));
      _fprintf(DAT_006f14b8,"Log file closed\n");
      _fclose(DAT_006f14b8);
      DAT_006f14b8 = (FILE *)0x0;
    }
    DAT_006869b8 = 0;
    return 0;
  }
  return 0xfffffffb;
}
#endif
```

## network_signal_quality_glyph.c

```
// network_signal_quality_glyph  (Ghidra: FUN_00440610, still unnamed -> renamed)
// address 0x440610, size 48 bytes
// name confidence: 0.25   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("maps a small integer enum value to
// a fixed output byte via a lookup switch, purpose unconfirmed"); `python tools/pack.py
// 0x440610`. The returned bytes (0x2b '+', 0x37 '7', 0x38 '8', 0x39 '9', 0x2e '.', 0x31 '1')
// are plain ASCII digits/punctuation, which in Blam's HUD font mapping typically select
// icon glyphs (e.g. connection-quality bar icons); this is a guess and not confirmed by any
// caller in this batch.
// register convention: lookup code in EAX (in_EAX), no stack arguments.
// UNSURE: the meaning of the input code and of each returned glyph byte; only the switch
// table itself is certain.
```

```
#if 0
Original Ghidra decompilation (0x440610):

undefined4 FUN_00440610(void)

{
  undefined4 in_EAX;

  switch(in_EAX) {
  default:
    return 0x2b;
  case 3:
    return 0x37;
  case 4:
    return 0x38;
  case 5:
    return 0x39;
  case 6:
    return 0x2e;
  case 8:
    return 0x31;
  }
}
#endif
```

## network_stats_summary_log_open.c

```
// network_stats_summary_log_open  (Ghidra: network_stats_summary_log_open, already named)
// address 0x440670, size 417 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md "bandwidth statistics" section names every
// global here (debug_log_level, network_statistics_logging_enabled,
// network_summary_log_needs_open, network_summary_log_file, network_summary_stats); the
// literal strings "\Game Summary " and the tab-separated header pin the rest.
// register convention: __cdecl, no arguments.
//
// Two simplifications from the raw decompile, both verified to produce identical resulting
// bytes (same technique as src/math/random_seed_generate.c folding __allmul/__alldiv):
//  1. The decompiled code builds a literal "Gamespy Metrics\0" string on the stack right
//     before the path buffer purely so that a later `p - 1` pointer (shown by Ghidra as
//     "local_20c + 3") has somewhere valid to start a find-end-of-string scan from. That text
//     is never read as data (the path buffer that follows it is overwritten first, and every
//     later scan starts one byte before the CURRENT contents of the path buffer, not before
//     the "Gamespy Metrics" text) -- see out/phase2/networking/00.md around this address for
//     the raw byte assignments. It has no observable effect and is not reproduced here.
//  2. The three manual "walk to the NUL, then dword/byte-copy" loops that follow are ordinary
//     strcpy/strcat, and are written as such.
// The base log-directory path comes from network_log_path_resolve (0x4e40a0; ESI = "Gamespy Metrics", not dead text)
// (its result is what gets a directory created for it, then has the log filename appended).
// UNSURE: FUN_00449210 (foreign) is used here as a zero-argument millisecond tick reader
// (QueryPerformanceCounter scaled by its frequency, same shape as random_seed_generate.c);
// note src/objects/object_nudge_position_by_velocity.c calls the same address with a visible
// pointer argument in a different context, which this rewrite does not attempt to reconcile.
// UNSURE: the exact contents of the fopen mode string at 0x0065fd30 (not captured by string
// extraction; assumed to be a plain text mode such as "w").
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)
```

```
#if 0
Original Ghidra decompilation (0x440670):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl network_stats_summary_log_open(void)

{
  char cVar1;
  undefined4 *puVar2;
  tm *_Tm;
  char *pcVar3;
  uint uVar4;
  uint uVar5;
  char *pcVar6;
  undefined4 *puVar7;
  undefined8 local_21c;
  undefined4 local_214;
  undefined4 local_210;
  undefined1 local_20c [4];
  char local_208 [4];
  undefined1 local_204 [10];
  char local_1fa [246];
  char local_104 [260];

  if ((2 < DAT_0087ac06) && (DAT_006f14b4 != '\0')) {
    if (DAT_006869bc != '\0') {
      local_214 = 0x20797073;
      local_21c._4_4_ = 0x656d6147;
      local_210 = 0x7274654d;
      local_20c = (undefined1  [4])&DAT_00736369;
      FID_conflict___time32((__time32_t *)&local_21c);
      _Tm = _localtime(&local_21c);
      _strftime(local_104,0x103,"%Y-%m-%d %H_%M_%S",_Tm);
      pcVar3 = (char *)FUN_004e40a0();
      pcVar6 = local_208;
      do {
        cVar1 = *pcVar3;
        pcVar3 = pcVar3 + 1;
        *pcVar6 = cVar1;
        pcVar6 = pcVar6 + 1;
      } while (cVar1 != '\0');
      directory_create_recursive(local_208);
      pcVar6 = local_20c + 3;
      do {
        pcVar3 = pcVar6;
        pcVar6 = pcVar3 + 1;
      } while (pcVar3[1] != '\0');
      builtin_strncpy(pcVar3 + 1,"\\Game Summary ",0xf);
      pcVar6 = local_104;
      do {
        cVar1 = *pcVar6;
        pcVar6 = pcVar6 + 1;
      } while (cVar1 != '\0');
      uVar4 = (int)pcVar6 - (int)local_104;
      pcVar6 = local_20c + 3;
      do {
        pcVar3 = pcVar6 + 1;
        pcVar6 = pcVar6 + 1;
      } while (*pcVar3 != '\0');
      pcVar3 = local_104;
      for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined4 *)pcVar6 = *(undefined4 *)pcVar3;
        pcVar3 = pcVar3 + 4;
        pcVar6 = pcVar6 + 4;
      }
      for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *pcVar6 = *pcVar3;
        pcVar3 = pcVar3 + 1;
        pcVar6 = pcVar6 + 1;
      }
      puVar2 = (undefined4 *)(local_20c + 3);
      do {
        puVar7 = puVar2;
        puVar2 = (undefined4 *)((int)puVar7 + 1);
      } while (*(char *)((int)puVar7 + 1) != '\0');
      *(undefined4 *)((int)puVar7 + 1) = 0x736c782e;
      *(undefined1 *)((int)puVar7 + 5) = 0;
      DAT_006a6140 = (FILE *)FUN_00624186(local_208,&DAT_0065fd30);
      _fprintf(DAT_006a6140,
               "Map\tLength (seconds)\tAvg # Players\tPackets Sent\tPackets Received\tPackets Sent/sec\tPackets Received/sec\tBytes Sent\tBytes Received\tBytes Sent/sec\tBytes Received/sec\tBits Sent/sec/conn\tBits Received/sec/conn\tBytes Sent/packet\tBytes Received/packet\n"
              );
      DAT_006869bc = '\0';
    }
    _DAT_0087bea0 = 0;
    DAT_0087bea4 = 0;
    DAT_0087bea8 = 0;
    DAT_0087beac = 0;
    DAT_0087beb0 = 0;
    _DAT_0087beb4 = 0;
    _DAT_0087beb8 = 0;
    _DAT_0087bea0 = FUN_00449210();
  }
  return;
}
#endif
```

## network_stats_summary_log_write.c

```
// network_stats_summary_log_write  (Ghidra: network_stats_summary_log_write, already named)
// address 0x440820, size 605 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md "bandwidth statistics" section; the fourteen
// "%f\t"/"%d\t" writes match, in order, the fourteen columns of the "Game Summary" header
// (network_stats_summary_log_open.c) that follow "Map" -- that column is written by this
// function's caller, not here.
// register convention: __cdecl, no arguments.
// Ghidra's "eight extra trailing arguments" on the first fprintf are stack residue: the
// disassembly shows exactly fourteen fprintf calls with one value each, in the column order used
// here (elapsed s, avg players, packets sent/recv (%d), pkts/s sent/recv, bytes sent/recv (%d),
// bytes/s sent/recv, kbits-style per-player rates x8, bytes per packet sent/recv), then fflush.
// Only rounding differs: the original keeps E = elapsed*0.001 and 1/E in x87 registers instead of
// rounding them to float; the intermediate float stores (avg, rates, per-packet) match.
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)

// VERIFIED against disassembly 0x440820..0x440a7d (2026-09-30): gate (level>=3, enabled, file open), x87 sequence, all 14 format strings and column order
```

```
#if 0
Original Ghidra decompilation (0x440820):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl network_stats_summary_log_write(void)

{
  float fVar1;
  float fVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;

  if (((2 < DAT_0087ac06) && (DAT_006f14b4 != '\0')) && (DAT_006a6140 != (FILE *)0x0)) {
    iVar3 = FUN_00449210();
    fVar1 = (float)(iVar3 - _DAT_0087bea0);
    if (iVar3 - _DAT_0087bea0 < 0) {
      fVar1 = fVar1 + 4.2949673e+09;
    }
    fVar6 = (float)_DAT_0087beb4 / (float)_DAT_0087beb8;
    fVar8 = 1.0 / (fVar1 * 0.001);
    fVar9 = fVar8 * (float)DAT_0087beac;
    fVar10 = (float)DAT_0087beb0 * fVar8;
    fVar7 = (float)DAT_0087bea4 * fVar8;
    fVar2 = (float)DAT_0087bea8;
    fVar8 = fVar2 * fVar8;
    fVar4 = (float)DAT_0087bea4 / (float)DAT_0087beac;
    fVar5 = fVar2 / (float)DAT_0087beb0;
    _fprintf(DAT_006a6140,"%f\t",(double)(fVar1 * 0.001),fVar4,fVar2,fVar5,fVar6,fVar7,fVar8,fVar9,
             fVar10);
    _fprintf(DAT_006a6140,"%f\t",(double)fVar6);
    _fprintf(DAT_006a6140,"%d\t",DAT_0087beac);
    _fprintf(DAT_006a6140,"%d\t",DAT_0087beb0);
    _fprintf(DAT_006a6140,"%f\t",(double)fVar9);
    _fprintf(DAT_006a6140,"%f\t",(double)fVar10);
    _fprintf(DAT_006a6140,"%d\t",DAT_0087bea4);
    _fprintf(DAT_006a6140,"%d\t",DAT_0087bea8);
    _fprintf(DAT_006a6140,"%f\t",(double)fVar7);
    _fprintf(DAT_006a6140,"%f\t",(double)fVar8);
    fVar6 = 1.0 / fVar6;
    _fprintf(DAT_006a6140,"%f\t",(double)(fVar6 * fVar7 * 8.0));
    _fprintf(DAT_006a6140,"%f\t",(double)(fVar6 * fVar8 * 8.0));
    _fprintf(DAT_006a6140,"%f\t",(double)fVar4);
    _fprintf(DAT_006a6140,"%f\n",(double)fVar5);
    _fflush(DAT_006a6140);
  }
  return;
}
#endif
```

## network_update.c

```
// network_update  (Ghidra: network_update, already named)
// address 0x4418d0, size 129 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("per-frame networking service
// routine: updates the high-resolution clock, drives the connection-statistics log, and pumps
// both network channels"); reuses network_game_socket/network_query_socket from
// networking_types_notes.md and performance_frequency from
// src/math/random_seed_generate.c (same QueryPerformanceCounter/__allmul/__alldiv shape,
// folded into plain int64_t arithmetic here for the same reason).
// register convention: __cdecl, no arguments.
// UNSURE: DAT_006869bf is not documented anywhere in networking_types_notes.md; named here
// only from its one-shot clear-if-set shape. gt2Think (channel pump) and gamespy_think_all are
// foreign GameSpy transport calls; the final `& 0xffff0000` return mask is preserved literally
// without a guess at its meaning.
```

```
#if 0
Original Ghidra decompilation (0x4418d0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint network_update(void)

{
  uint uVar1;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  _DAT_006a6144 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  if (DAT_006869bf == '\x01') {
    DAT_006869bf = '\0';
  }
  network_connection_stats_log_tick();
  if (DAT_006f14c4 != 0) {
    FUN_00614540(DAT_006f14c4);
  }
  if (DAT_006f14c8 != 0) {
    FUN_00614540(DAT_006f14c8);
  }
  uVar1 = FUN_006154f0();
  return uVar1 & 0xffff0000;
}
#endif
```
