# Original notes: networking `net1_server`

Author notes and decompile blocks moved verbatim from the C files converted into the net1_server sources.

## network_channel_dispatch_bitstream_unit.c

```
// network_channel_dispatch_bitstream_unit  (Ghidra: FUN_004e18b0, unnamed)
// address 0x4e18b0, size 119 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (REWRITTEN; was 0.45)
// evidence: out/phase4/networking_functions.md: "Small dispatcher used while draining a
// channel's bitstream, routing each unit to either the queued-message processor or the
// incoming-packet decoder." network_game_process_incoming_message is already named and in
// this batch.
// register convention: EAX read at entry is the single genuine stack argument, `unit`, cached
// into EBP (survives the calls); `machine` (network_machine*) is a live-in forwarded through ESI
// -- never set locally -- straight into network_game_process_incoming_message's own verified
// ECX -> machine slot at its call site (0x4e190b `mov ecx,esi`).
// blam-cc: ECX -> stream, ESI -> machine, stack -> server, unit
// FIXED (register inputs, objdump): ESI is a genuine live-in (0x4e18ce `push esi`,
// 0x4e190b `mov ecx,esi`) that the notes did not map; added as `machine`. While tracing that
// call, also found the previously-modeled `server` stack parameter does not exist in the binary
// -- only one stack slot (`unit`) is ever read (0x4e18ba/0x4e18c5) -- and that this function
// discarded FUN_004de420's return value instead of forwarding it as `record`/`length` the way
// network_game_process_incoming_message's own verified convention requires; both are fixed below.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4e18b0..0x4e1927): besides (server, unit)
// on the stack it takes the item's stream in ECX and the machine in ESI. A game-action item (1) drains through
// network_client_drain_queued_updates (ECX stream; stack server, machine); a message item (0) is read into a local
// 0x1000-byte buffer (EDI buffer, EBX stream, capacity 0xfff) and handed to network_game_process_incoming_message
// (EAX length = first word >> 4, ECX machine, EDX record, stack server). The previous C passed the unit flag as the
// server and no stream.

// VERIFIED against disassembly 0x4e18b0..0x4e1927 (2026-09-30): ESI machine, ECX stream, stack (server, unit); unit 1 -> drain_queued_updates(ECX stream; server, machine); unit 0 -> read_sized_buffer then process_incoming_message(EAX length, ECX machine, EDX record, stack server)
```

```
#if 0
Original Ghidra decompilation (0x4e18b0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_004e18b0(undefined4 param_1,uint param_2)

{
  uint uVar1;
  int iVar2;

  if (param_2 == 1) {
    uVar1 = FUN_004e1f40(param_1);
    return uVar1;
  }
  if (param_2 == 0) {
    iVar2 = FUN_004de420(0xfff);
    param_2 = 0;
    if (iVar2 != 0) {
      uVar1 = network_game_process_incoming_message(param_1);
      return uVar1;
    }
  }
  return param_2 & 0xffffff00;
}
#endif
```

## network_channel_drain_bitstream.c

```
// network_channel_drain_bitstream  (Ghidra: FUN_004e1290, unnamed)
// address 0x4e1290, size 375 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (REWRITTEN; was 0.25)
// evidence: out/phase4/networking_functions.md: "Drains a shared bitstream buffer one bit at a
// time, dispatching each bit through FUN_004e18b0 as part of connecting a new machine."
// *param_2 (machine->channel) and channel->incoming (channel+0xc) match
// network_channel::endpoint/incoming... wait, channel+0xc is actually ::incoming per
// types/networking.h; channel+0x8/+0xc/+0x10 on the *circular_buffer* itself match
// read_cursor/write_cursor/capacity. FUN_004dcf10 is functions.md's
// "network_channel_incoming_read_item" (already named there, conf 0.4, not renamed here since
// it is outside this batch's address range).
// register convention: both parameters are genuine stack (cdecl) parameters per Ghidra's own
// signature.
// blam-cc: stack -> server, machine
// UNSURE: transcribed very literally (Ghidra's own bit-cursor locals renamed but not
// restructured into a clean bit_stream abstraction), matching the precedent set by
// src/networking/network_game_process_incoming_messages.c for this exact style of
// hand-inlined bit walk, since the edge-case boundary behaviour is delicate and not
// independently re-derivable with confidence.
// UNSURE: `network_incoming_message_scratch` (DAT_00861de0) and its structure are only known
// by address reuse with that same file; no declared type exists for it in types/networking.h.
// UNSURE: FUN_004dcf10's 5th (24-byte) output parameter is unused after the call in this
// function and is not otherwise interpreted here.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4e1290..0x4e1407): the host twin of
// network_game_process_incoming_messages. While the machine's channel (machine +0) has queued data, each item is
// read (network_channel_incoming_read_item, max 0x80000 bits in EAX) into 0x861de0 and walked as a local bit stream;
// each leading bit goes to network_channel_dispatch_bitstream_unit(server, bit) with the stream (ECX) and the
// machine (ESI). A failed read, or a dispatch returning 0, ends the drain with 0; an empty queue returns 1.
```

```
#if 0
Original Ghidra decompilation (0x4e1290):

undefined1 FUN_004e1290(undefined4 param_1,int *param_2)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  bool bVar4;
  char cVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  int local_3c;
  uint local_38;
  undefined1 local_34 [24];
  undefined4 local_1c;
  undefined *local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  uint local_8;
  int local_4;

  do {
    iVar1 = *(int *)(*param_2 + 0xc);
    if (iVar1 == 0) {
      return 1;
    }
    iVar8 = *(int *)(iVar1 + 0xc) - *(int *)(iVar1 + 8);
    if (iVar8 < 0) {
      iVar8 = iVar8 + *(int *)(iVar1 + 0x10);
    }
    if (iVar8 == 0) {
      return 1;
    }
    local_38 = 0;
    local_3c = 0;
    cVar5 = FUN_004dcf10(*param_2,&DAT_00861de0,&local_38,&local_3c,local_34);
    if (cVar5 == '\0') {
      return 0;
    }
    local_c = local_38 & 7;
    local_10 = local_38 >> 3;
    local_8 = local_3c + -1 + local_38;
    local_1c = 1;
    local_18 = &DAT_00861de0;
    uVar7 = local_c;
    uVar6 = local_10;
    uVar2 = local_8;
    local_14 = local_38;
    local_4 = local_3c;
    uVar3 = local_38;
    while ((cVar5 == '\x01' && (7 < ((uVar3 + uVar6 * -8) - uVar7) + local_3c))) {
      bVar4 = false;
      uVar9 = uVar6 * 8 + uVar7;
      local_38 = 0;
      if ((uVar3 <= uVar9) && (uVar9 <= uVar2)) {
        local_38 = (int)((uint)(byte)local_18[uVar6] & 1 << ((byte)uVar7 & 0x1f)) >>
                   ((byte)uVar7 & 0x1f) & 0xff;
        uVar9 = uVar9 + 1;
        if (((uVar3 <= uVar9) && (uVar9 <= uVar2)) || (uVar9 == uVar2 + 1)) {
          uVar7 = uVar9 & 7;
          uVar6 = uVar9 >> 3;
          local_10 = uVar6;
          local_c = uVar7;
        }
        bVar4 = true;
      }
      cVar5 = '\0';
      if (bVar4) {
        cVar5 = FUN_004e18b0(param_1,local_38);
        uVar7 = local_c;
        uVar6 = local_10;
        uVar2 = local_8;
        uVar3 = local_14;
      }
    }
    local_1c = 0xffffffff;
    local_18 = (undefined *)0x0;
    local_14 = 0;
    local_10 = 0;
    local_c = 0;
    local_8 = 0;
    local_4 = 0;
  } while (cVar5 != '\0');
  return 0;
}
#endif
```

## network_game_all_machines_have_player.c

```
// network_game_all_machines_have_player  (Ghidra: FUN_004e04f0, unnamed)
// address 0x4e04f0, size 145 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Verifies that every team/channel key stored
// at param_1+0x3c4 has a corresponding valid, active player-name entry, used as a
// synchronisation gate before starting a round." param_1+0x3c4 and +0x1c6 match
// network_server_globals::machines[0].machine_id and ::session.players[0].machine_index
// exactly.
// register convention: stack = server (network_server_globals *).
// blam-cc: stack -> server
// UNSURE: as in network_game_any_team_empty.c, network_player_entry_is_valid's EAX argument
// has no visible source in this function; modelled as an explicit `entry` walking the player
// table in lockstep with the byte pointer Ghidra does show.
```

```
#if 0
Original Ghidra decompilation (0x4e04f0):

uint FUN_004e04f0(int param_1)

{
  short sVar1;
  bool bVar2;
  uint uVar3;
  char *pcVar4;
  int iVar5;
  short *local_8;
  int local_4;

  local_8 = (short *)(param_1 + 0x3c4);
  local_4 = 0;
  do {
    sVar1 = *local_8;
    if ((-1 < sVar1) && (sVar1 < 0x10)) {
      bVar2 = false;
      pcVar4 = (char *)(param_1 + 0x1c6);
      iVar5 = 0x10;
      do {
        uVar3 = FUN_004de9f0();
        if (((char)uVar3 != '\0') && (*pcVar4 == sVar1)) {
          bVar2 = true;
        }
        pcVar4 = pcVar4 + 0x20;
        iVar5 = iVar5 + -1;
      } while (iVar5 != 0);
      if (!bVar2) {
        return uVar3 & 0xffffff00;
      }
    }
    local_4 = local_4 + 1;
    local_8 = local_8 + 0x30;
    if (0xf < local_4) {
      return CONCAT31((int3)((uint)local_4 >> 8),1);
    }
  } while( true );
}
#endif
```

## network_game_any_team_empty.c

```
// network_game_any_team_empty  (Ghidra: FUN_004e0480, unnamed)
// address 0x4e0480, size 104 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md describes this as "Checks whether both teams
// (0 and 1) have at least one active, validated player, returning true only when the game is
// in team mode and both teams are populated." Traced literally, the polarity is the opposite:
// the function returns 1 as soon as it finds a team (0 or 1) with a zero count, and only
// returns 0 once both team counts are confirmed non-zero (or the game is not in team mode at
// all). in_EAX+0x140 is exactly network_server_globals::session.variant.teams
// (session at +0x008, game_variant at session+0x104, teams at game_variant+0x34); the
// per-entry field at +0x1e (unknown_1e) is read here as a 0/1 team index.
// register convention: EAX = server (network_server_globals *).
// blam-cc: EAX -> server
// UNSURE: entry->unknown_1e is read as a team index here, which is new information not
// reflected in that field's name in types/networking.h (left unrenamed; header not edited).
// UNSURE: EAX is re-read as the implicit "entry" argument to network_player_entry_is_valid on
// every loop iteration but this function's own body never shows an entry pointer being
// advanced; modelled here with an explicit `entry` walking the player table in lockstep with
// the byte pointer Ghidra does show, consistent with the analogous pattern in
// network_game_server_handoff_object_ownership.c.
```

```
#if 0
Original Ghidra decompilation (0x4e0480):

undefined4 FUN_004e0480(void)

{
  char cVar1;
  int in_EAX;
  char *pcVar2;
  int iVar3;
  short local_4 [2];

  if (*(char *)(in_EAX + 0x140) != '\0') {
    local_4[0] = 0;
    local_4[1] = 0;
    pcVar2 = (char *)(in_EAX + 0x1c8);
    iVar3 = 0x10;
    do {
      cVar1 = FUN_004de9f0();
      if (((cVar1 != '\0') && (cVar1 = *pcVar2, -1 < cVar1)) && (cVar1 < '\x02')) {
        local_4[cVar1] = local_4[cVar1] + 1;
      }
      pcVar2 = pcVar2 + 0x20;
      iVar3 = iVar3 + -1;
    } while (iVar3 != 0);
    iVar3 = 0;
    do {
      if (local_4[iVar3] == 0) {
        return 1;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < 2);
  }
  return 0;
}
#endif
```

## network_game_broadcast_player_set_changed.c

```
// network_game_broadcast_player_set_changed  (Ghidra: FUN_004e1bf0; named per this rewrite)
// address 0x4e1bf0, size 103 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Posts a type-0x21 game-engine event and, if
// accepted, broadcasts an associated update packet to the whole session -- used whenever the
// connected-player set changes." 0x00871de0 is types/networking.h's shared encode scratch
// buffer, already named network_message_scratch by
// src/networking/network_server_check_machine_timeout.c (same address, "shared with
// network_game_broadcast_team_object_updates.c").
// register convention: ECX = server (implicit passthrough, following the same pattern
// established by network_game_broadcast_state_snapshot.c's disassembly-verified ESI -> server
// forwarding for the sibling message-0x17 broadcaster), stack = param_1.
//   // blam-cc: ECX -> server, stack -> param_1
// UNSURE: `param_1` is only ever used to compute `&(param_1 + 8)`, itself only used as an
// address-of-a-local passed to message_delta_encode_message; its true type and the meaning of
// the +8 adjustment are not recoverable from this function alone.
// UNSURE: message_delta_encode_message's own destination is not visible in this call (no
// output buffer argument); the subsequent broadcast reads network_message_scratch
// directly, so the encoder is presumed to write there through a fixed/global convention this
// function does not itself set up.
// UNSURE: `server` being implicit (rather than a genuine parameter) is inferred by analogy to
// network_game_broadcast_state_snapshot.c's disassembly-verified case, not independently
// re-checked here.
```

```
#if 0
Original Ghidra decompilation (0x4e1bf0), from tools/pack.py 0x4e1bf0:

bool FUN_004e1bf0(void *param_1)

{
  int iVar1;

  message_delta_parameters_protocol_send_update();
  param_1 = (void *)((int)param_1 + 8);
  iVar1 = message_delta_encode_message(0,0x21,0,&param_1,0,1,'\0');
  if (0 < iVar1) {
    FUN_004e19c0(1,&DAT_00871de0,1,0,1,3);
  }
  return 0 < iVar1;
}
#endif
```

## network_game_broadcast_state_snapshot.c

```
// network_game_broadcast_state_snapshot  (Ghidra: FUN_004e1b50; named per this rewrite)
// address 0x4e1b50, size 156 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Packages a small fixed-size game-state
// structure into message type 0x17 and broadcasts it to all established machines." Follows the
// exact same data_packet_group_encode_packet / network_message_block_build / broadcast idiom as
// src/networking/network_game_settings_broadcast_send.c (message type 0x18), confirmed here by
// disassembly (objdump -d -M intel, bin/halo.exe):
//   4e1b59: mov esi,eax                 ; ESI = record (EAX in), kept resident across the call
//   4e1b9e: mov eax,0x6b7f98            ; network_message_block_build(dest=network_challenge_packet_block,
//   4e1ba3: lea ecx,[esp+0x2c]          ;   buffer=encoded body, flags(dl)=3, length=edx)
//   4e1ba7: mov dl,0x3
//   4e1ba9: call 0x440350
//   4e1bc3: mov ecx,[esp+0x640]         ; reloads the CALLER's original ESI (saved by this
//                                       ; function's own `push esi` in its prologue, at
//                                       ; exactly this stack depth) as the `server` argument
//                                       ; to network_session_broadcast_to_all -- i.e. `server`
//                                       ; arrives in ESI, is never touched by this function's
//                                       ; body (ESI is repurposed for `record` instead), and is
//                                       ; recovered from the saved-register stack slot.
// recovered from the prologue's own register-save slot, not read directly in the C body).
// UNSURE: the broadcast's remaining fixed arguments (0, 1, 0, 1, 3) are transcribed literally
// from both Ghidra's decompile and the disassembly above; their individual meanings are not
// independently re-derived here (see network_session_broadcast_to_all.c's own UNSURE notes for
// what little is known about that parameter list).
```

```
#if 0
Original Ghidra decompilation (0x4e1b50), from tools/pack.py 0x4e1b50:

uint FUN_004e1b50(void)

{
  undefined4 *in_EAX;
  uint uVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 local_624;
  undefined4 local_620 [392];

  puVar3 = local_620;
  for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = *in_EAX;
    in_EAX = in_EAX + 1;
    puVar3 = puVar3 + 1;
  }
  local_624 = 0x600;
  uVar1 = data_packet_group_encode_packet(local_620,&local_624,0x17,1);
  if ((char)uVar1 != '\0') {
    iVar2 = FUN_00440350(local_624);
    uVar1 = 0;
    if (iVar2 != 0) {
      uVar1 = FUN_004e19c0(0,iVar2,1,0,1,3);
      return uVar1;
    }
  }
  return uVar1 & 0xffffff00;
}

Disassembly (objdump -d -M intel, bin/halo.exe) confirming the register convention and the two
under-attributed calls:
  4e1b59: mov esi,eax                 ; ESI = record
  4e1b9e: mov eax,0x6b7f98
  4e1ba3: lea ecx,[esp+0x2c]
  4e1ba7: mov dl,0x3
  4e1ba9: call 0x440350               ; network_message_block_build(0x6b7f98, buffer, 3, length)
  4e1bc3: mov ecx,DWORD PTR [esp+0x640] ; server, from the saved-ESI prologue slot
  4e1bd2: call 0x4e19c0               ; network_session_broadcast_to_all(server, 0, block, 1, 0, 1, 3)
#endif
```

## network_game_client_game_settings_updated.c

```
// network_game_client_game_settings_updated  (Ghidra: network_game_client_game_settings_updated,
// already named)
// address 0x4df2e0, size 541 bytes
// name confidence: 0.6   rewrite confidence: 0.15
// evidence: out/phase4/networking_functions.md: "Handles a game-settings update for a new
// network round: resets channel and history tables, applies the current game variant/defaults,
// loads the requested map (network_game_server_load_scenario), and kicks off either the host or client path
// depending on host->flags bit2." Confirmed field matches: host->flags bit2 (+6), the
// session.players[] reset loop at +0x1c7 (== session+0x1bf, byte-for-byte the same pattern as
// network_game_session_reset.c), the machines[] loop at +0x3c6 (== machines[0]+0xe, the flags
// byte, clearing bit2 and the unknown_04/unknown_08/unknown_50 fields byte-by-byte -- collapsed
// here to whole-field writes), and the +0x3b0 round counter shared with
// network_host_round_reset.c.
// UNSURE (significant): this is the largest and least-verified file in the batch. Most of the
// UI/global-state fields (DAT_00718f8c/94/98/a6, DAT_0087aa40/80/84, DAT_006953e8,
// DAT_00712542/44, DAT_0087a478/80) are foreign to types/networking.h and are declared
// generically; the byte-level struct-copy loops are preserved as raw memcpy/offset writes
// rather than typed field assignments where no field mapping could be confirmed.
// register convention: fully recovered cdecl (the host/session container is the only parameter).
```

```
#if 0
Original Ghidra decompilation (0x4df2e0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint __cdecl network_game_client_game_settings_updated(int *param_1)

{
  byte *pbVar1;
  undefined1 *puVar2;
  uint uVar3;
  int iVar4;
  int *piVar5;
  int *piVar6;
  undefined4 *puVar7;

  param_1[0x272] = 0;
  param_1[0x273] = 0;
  param_1[0x274] = 0;
  param_1[0x275] = 0;
  if ((*(byte *)((int)param_1 + 6) >> 2 & 1) == 0) {
    if (*(int *)(DAT_0087a478 + 4) != -1) {
      iVar4 = datum_get();
      if (iVar4 != 0) {
        *(undefined4 *)(DAT_0071c2d8 + 0xf10) = *(undefined4 *)(iVar4 + 0x20);
      }
    }
  }
  else {
    message_delta_parameters_protocol_dump_to_config_file();
    network_stats_summary_log_write();
    message_delta_protocol_initialize();
    network_stats_summary_log_open();
  }
  param_1[0xec] = param_1[0xec] + 1;
  param_1[0x26e] = 0;
  param_1[0x271] = 0;
  *(undefined1 *)((int)param_1 + 0x9f9) = 0;
  *(undefined1 *)((int)param_1 + 0x9fa) = 0;
  *(undefined1 *)(param_1 + 0x27e) = 0;
  pbVar1 = (byte *)((int)param_1 + 0x3c6);
  iVar4 = 0x10;
  do {
    *pbVar1 = *pbVar1 & 0xfb;
    pbVar1[-0xffffffff0000000a] = 0;
    pbVar1[-0xffffffff00000009] = 0;
    pbVar1[-0xffffffff00000008] = 0;
    pbVar1[-0xffffffff00000007] = 0;
    pbVar1[-0xffffffff00000006] = 0;
    pbVar1[-0xffffffff00000005] = 0;
    pbVar1[-0xffffffff00000004] = 0;
    pbVar1[-0xffffffff00000003] = 0;
    pbVar1[0x42] = 0;
    pbVar1 = pbVar1 + 0x60;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  piVar5 = param_1 + 0x22;
  for (iVar4 = 0x21; iVar4 != 0; iVar4 = iVar4 + -1) {
    *piVar5 = 0;
    piVar5 = piVar5 + 1;
  }
  piVar5 = param_1 + 0x43;
  for (iVar4 = 0x26; iVar4 != 0; iVar4 = iVar4 + -1) {
    *piVar5 = 0;
    piVar5 = piVar5 + 1;
  }
  *(undefined2 *)(param_1 + 0x6a) = 0;
  puVar2 = (undefined1 *)((int)param_1 + 0x1c7);
  iVar4 = 0x10;
  do {
    puVar2[-1] = 0xff;
    *puVar2 = 0xff;
    puVar2[1] = 0xff;
    puVar2[2] = 0xff;
    *(undefined2 *)(puVar2 + -0x1d) = 0;
    *(undefined2 *)(puVar2 + -5) = 0xffff;
    *(undefined2 *)(puVar2 + -3) = 0xffff;
    puVar2 = puVar2 + 0x20;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  param_1[0xed] = 0;
  *(undefined2 *)(param_1 + 1) = 0;
  game_engine_apply_current_custom_variant();
  FUN_0045fc80();
  if ((*(byte *)((int)param_1 + 6) >> 2 & 1) == 0) {
    DAT_00718f8c = 2;
  }
  *(undefined1 *)((int)param_1 + 0xa0e) = 1;
  piVar5 = &DAT_0087aa80;
  piVar6 = param_1 + 0x43;
  for (iVar4 = 0x26; iVar4 != 0; iVar4 = iVar4 + -1) {
    *piVar6 = *piVar5;
    piVar5 = piVar5 + 1;
    piVar6 = piVar6 + 1;
  }
  _strncpy((char *)(param_1 + 0x23),&DAT_0087aa40,0x3f);
  *(undefined1 *)((int)param_1 + 0xcb) = 0;
  *(ushort *)((int)param_1 + 6) = *(ushort *)((int)param_1 + 6) | 1;
  *(undefined2 *)((int)param_1 + 0x86) = 0;
  param_1[0x22] = 0;
  *(undefined1 *)(*param_1 + 0xae0) = 1;
  if (DAT_00718f94 != 0) {
    widget_close(DAT_00718f94);
  }
  if (DAT_00718f98 != 0) {
    FUN_004994b0();
  }
  _DAT_00718fa6 = 0;
  if (DAT_006953e8 != -1) {
    DAT_00712542 = DAT_00712542 & 0xf7;
    puVar7 = &DAT_00712544;
    for (iVar4 = 0xa0; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    DAT_006953e8 = -1;
  }
  *(undefined1 *)((int)param_1 + 0x9d5) = 0;
  uVar3 = FUN_004e0720();
  if ((char)uVar3 == '\x01') {
    *(undefined2 *)(param_1 + 1) = 1;
    if ((*(byte *)((int)param_1 + 6) >> 2 & 1) != 0) {
      uVar3 = FUN_004df510(param_1);
      *(undefined1 *)((int)param_1 + 0x9fa) = 0;
      return uVar3 & 0xffffff00;
    }
    uVar3 = FUN_004d9ed0(0,0);
  }
  *(undefined1 *)((int)param_1 + 0x9fa) = 0;
  return uVar3 & 0xffffff00;
}
#endif
```

## network_game_client_handle_map_data.c

```
// network_game_client_handle_map_data  (Ghidra: FUN_004e2790; named per this rewrite)
// address 0x4e2790, size 126 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.25)
// evidence: out/phase4/networking_functions.md's summary claims message type 0x1b, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x1c: FUN_004e2790(param_1);` -- type 0x1b is instead handled inline inside the
// dispatcher itself (decode + a direct call to network_game_client_apply_position_update). The
// dispatcher's literal call table is trusted here instead of the low-confidence (0.3) summary.
// This function latches an incoming 32-byte game/map data block into the client's pending-state
// fields.
// register convention: stack = context (param_1, checked against state 1 and read/written at
// +0x9d8/+0x9f8), EDX = buffer (implicit passthrough), stack = length (param_2, only ever used
// as `length - 2`, its result unused beyond that -- dead by the time of the actual decode,
// which reads through the implicit EDX buffer instead).
//   // blam-cc: EDX -> buffer, stack -> context, length
// UNSURE (major): the summary calls this "client-side", but +0x9d8/+0x9f8 fall inside both
// types/networking.h's network_server_globals::unknown_9bc[0x3c] span AND
// network_client_globals::unknown_002[0xab2] span (both still-unresolved regions) -- nothing in
// this function's own body disambiguates which struct `context` really is, so it is kept as a
// raw byte pointer with explicit offset casts rather than asserting either type.
// UNSURE: network_player_entry_is_valid (FUN_004de9f0) is called here with zero visible
// arguments, against its own presumed EAX-based convention documented elsewhere in this batch.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2790: stack (server, length), EDX record, ESI machine; state (+4) 1, class 5. A decoded 0x20-byte player
// entry that validates (network_player_entry_validate, EAX) is stored at server +0x9d8 once (+0x9f8 set).
```

```
#if 0
Original Ghidra decompilation (0x4e2790), from tools/pack.py 0x4e2790:

undefined4 FUN_004e2790(int param_1,int param_2)

{
  int iVar1;
  char cVar2;
  int iVar3;
  int in_EDX;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined1 local_24 [4];
  undefined4 local_20 [8];

  iVar1 = param_1;
  if (*(short *)(param_1 + 4) == 1) {
    param_2 = param_2 + -2;
    cVar2 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,local_24,
                       &param_1,5);
    if ((cVar2 != '\0') && (*(char *)(iVar1 + 0x9f8) == '\0')) {
      cVar2 = FUN_004de9f0();
      if (cVar2 != '\0') {
        puVar4 = local_20;
        puVar5 = (undefined4 *)(iVar1 + 0x9d8);
        for (iVar3 = 8; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar5 = *puVar4;
          puVar4 = puVar4 + 1;
          puVar5 = puVar5 + 1;
        }
        *(undefined1 *)(iVar1 + 0x9f8) = 1;
        return 1;
      }
    }
  }
  return 1;
}
#endif
```

## network_game_client_handle_retry_schedule.c

```
// network_game_client_handle_retry_schedule  (Ghidra: FUN_004e2870; named per this rewrite)
// address 0x4e2870, size 83 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md's summary claims message type 0x1d, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x1e: FUN_004e2870();` (case 0x1d instead reaches FUN_004e2810, the settings relay) --
// the dispatcher's literal call table is trusted here instead of the low-confidence (0.3)
// summary. Forwards to network_machine_timer_start; the client-side counterpart of the
// FUN_004e2630 server handler, gated on role == 1 and decode class 5.
// register convention: EAX = context (implicit passthrough), ESI = machine (implicit
// passthrough, needed only for the network_machine_timer_start call), stack = buffer.
//   // blam-cc: EAX -> context, ESI -> machine, stack -> buffer
// UNSURE: `machine` is not read anywhere in this function's own body; inferred purely from
// network_machine_timer_start's own established ESI convention, per the same reasoning as
// network_game_message_handle_retry_schedule.c.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2870: EAX server, ESI machine, stack (record, length); state (+4) 1, class 5; then the machine's
// timer starts (network_machine_timer_start: ESI machine, stack 0).
```

```
#if 0
Original Ghidra decompilation (0x4e2870), from tools/pack.py 0x4e2870:

undefined4 FUN_004e2870(int param_1)

{
  char cVar1;
  int in_EAX;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  if (*(short *)(in_EAX + 4) == 1) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_4,param_1 + 2,local_8,
                       local_c,5);
    if (cVar1 != '\0') {
      FUN_004df090(0);
    }
  }
  return 1;
}
#endif
```

## network_game_client_handle_settings_relay.c

```
// network_game_client_handle_settings_relay  (Ghidra: FUN_004e2810; named per this rewrite)
// address 0x4e2810, size 88 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md's summary claims message type 0x1c, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x1d: FUN_004e2810();` (case 0x1c instead reaches FUN_004e2790, the map-data latch) --
// the dispatcher's literal call table is trusted here instead of the low-confidence (0.3)
// summary. Forwards to network_game_settings_broadcast_send, the same forward the server-side
// FUN_004e24d0 handler makes, but gated on role == 1 (client) and decode class 5 instead of 3.
// register convention: ESI = context (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> context, EDX -> buffer
// UNSURE: network_game_settings_broadcast_send is called here with zero visible arguments;
// matching Ghidra literally.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2810: ESI server, EDX record, stack length; state (+4) 1, class 5.
```

```
#if 0
Original Ghidra decompilation (0x4e2810), from tools/pack.py 0x4e2810:

undefined4 FUN_004e2810(void)

{
  char cVar1;
  undefined4 uVar2;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_28 [4];
  undefined1 local_24 [4];
  undefined1 local_20 [32];

  if (*(short *)(unaff_ESI + 4) == 1) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,local_24,
                       local_28,5);
    if (cVar1 != '\0') {
      uVar2 = FUN_004df0e0();
      return uVar2;
    }
  }
  return 1;
}
#endif
```

## network_game_message_handle_build_version.c

```
// network_game_message_handle_build_version  (Ghidra: FUN_004e2630; named per this rewrite)
// address 0x4e2630, size 100 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md's summary claims message types 0x14/0x25, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x15: FUN_004e2630();` -- the low-confidence (0.3) summary swapped this function's type
// number with FUN_004e26a0's (which the same switch shows at `case 0x14: case 0x25:`); the
// dispatcher's literal call table is trusted here instead. Forwards to
// network_machine_check_build_version (FUN_004dff20, already written: EAX -> remote_version,
// EDI -> machine).
// register convention: EAX = server (implicit passthrough), EDI = machine (forwarded unchanged
// to network_machine_check_build_version), stack = buffer (a genuine parameter, matching
// Ghidra's own recovered `int param_1`).
//   // blam-cc: EAX -> server, EDI -> machine, stack -> buffer
// FIXED (register inputs, objdump): EDI carries machine (read only by the tail call to
// network_machine_check_build_version at 0x4e2686, which needs EDI -> machine per its own
// file); it was missing entirely, and that call had zero visible arguments. objdump also shows
// the call's EAX argument is not the caller's own EAX but `lea eax,[esp+8]` taken right after
// the data_packet_group_decode_packet call's own stack cleanup, which lands at the base of this
// function's decoded_body local (frame layout: local_108/local_104 (out_a/out_b) at
// [esp+0..8), decoded_body at [esp+8..0x108)) -- i.e. remote_version is decoded_body itself.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2630: EAX server, EDI machine, stack (record, length); state (+4) 0, class 3; the decoded 0x100-byte
// version string goes to network_machine_check_build_version (EAX string, EDI machine).
```

```
#if 0
Original Ghidra decompilation (0x4e2630), from tools/pack.py 0x4e2630:

undefined4 FUN_004e2630(int param_1)

{
  char cVar1;
  int in_EAX;
  undefined1 local_108 [4];
  undefined1 local_104 [4];
  undefined1 local_100 [256];

  if (*(short *)(in_EAX + 4) == 0) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_100,param_1 + 2,local_108,
                       local_104,3);
    if (cVar1 != '\0') {
      FUN_004dff20();
    }
  }
  return 1;
}
#endif
```

## network_game_message_handle_handshake_forward.c

```
// network_game_message_handle_handshake_forward  (Ghidra: FUN_004e25e0; named per this rewrite)
// address 0x4e25e0, size 74 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md: "Handles message type 0x13 by decoding it and
// forwarding to FUN_004e0590" -- network_client_connection_handshake_tick, already written in
// an earlier batch.
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> server, EDX -> buffer
// UNSURE: network_client_connection_handshake_tick is called here with zero visible arguments,
// against its own file's (state, owner) signature; matching Ghidra literally.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e25e0: ESI server, EDX record, stack length; state (+4) 0, class 3; the decoded state goes to
// network_client_connection_handshake_tick (EAX state, ECX server).
```

```
#if 0
Original Ghidra decompilation (0x4e25e0), from tools/pack.py 0x4e25e0:

undefined4 FUN_004e25e0(void)

{
  char cVar1;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  if (*(short *)(unaff_ESI + 4) == 0) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_4,in_EDX + 2,local_8,
                       local_c,3);
    if (cVar1 != '\0') {
      FUN_004e0590();
    }
  }
  return 1;
}
#endif
```

## network_game_message_handle_join_finalize_ack_role2.c

```
// network_game_message_handle_join_finalize_ack_role2  (Ghidra: FUN_004e2930; named per this
// rewrite)
// address 0x4e2930, size 86 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md's summary claims message type 0x23, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x24: FUN_004e2930();` ('$' is 0x24, not 0x23) -- the dispatcher's literal call table
// is trusted here instead of the low-confidence (0.3) summary. The cleared bit
// (machine->flags &= ~0x04) is exactly the bit network_game_server_handle_client_join.c sets
// (`machine->flags |= 0x04`) while a join is pending, so this is that join's client-side
// finalize acknowledgement.
// register convention: ECX = context (role, checked against 2), ESI = machine (the flags
// clear), stack = buffer.
//   // blam-cc: ECX -> context, ESI -> machine, stack -> buffer
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2930: ECX server, ESI machine, stack (record, length); state (+4) 2, class 7; then the machine's flag
// byte +0x0e loses bit 4.
```

```
#if 0
Original Ghidra decompilation (0x4e2930), from tools/pack.py 0x4e2930:

undefined4 FUN_004e2930(int param_1)

{
  char cVar1;
  int in_ECX;
  int unaff_ESI;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  if (*(short *)(in_ECX + 4) == 2) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_4,param_1 + 2,local_8,
                       local_c,7);
    if (cVar1 != '\0') {
      *(byte *)(unaff_ESI + 0xe) = *(byte *)(unaff_ESI + 0xe) & 0xfb;
      return 1;
    }
  }
  return 1;
}
#endif
```

## network_game_message_handle_keepalive.c

```
// network_game_message_handle_keepalive  (Ghidra: FUN_004e2110; named per this rewrite)
// address 0x4e2110, size 177 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Handles message type 1 by building and sending
// a timestamped acknowledgement/challenge packet back to the sender's channel." Called from
// network_game_process_incoming_message's own `case 1: ... FUN_004e2110(&local_2c);` with the
// just-decoded record. `*channel + 0xa8c` matches network_channel::flags exactly.
// register convention: EAX = channel (network_channel **, may be NULL), stack = record (the
// just-decoded message body). Confirmed by disassembly (objdump -d -M intel, bin/halo.exe),
// which also recovers the two under-attributed calls Ghidra's own decompile dropped:
//   4e2170: mov eax,0x3            ; network_prepare_challenge_packet(message_type=3,
//   4e216c: lea edx,[esp+0x14]     ;   payload=&{echoed_value, timestamp_ms})
//   4e2175: call 0x4deaf0
// register convention: EAX -> channel, stack -> record.
//   // blam-cc: EAX -> channel, stack -> record
// UNSURE: network_channel_reliable_pool_store is called here with only 4 of its fuller, separately-reconstructed
// 6-argument signature (see network_channel_queue_message.c); called here with a matching
// 4-argument local prototype instead of forcing the mismatch, per the same reasoning documented
// in network_send_join_request_packet.c for data_packet_group_encode_packet.
```

```
#if 0
Original Ghidra decompilation (0x4e2110), from tools/pack.py 0x4e2110:

undefined4 FUN_004e2110(undefined4 *param_1)

{
  int iVar1;
  int *in_EAX;
  int iVar2;
  undefined8 uVar3;
  undefined1 local_11;
  LARGE_INTEGER local_10;
  undefined4 local_8;
  undefined4 local_4;

  if (in_EAX == (int *)0x0) {
    return 0;
  }
  iVar1 = *in_EAX;
  if (iVar1 != 0) {
    QueryPerformanceCounter(&local_10);
    local_8 = *param_1;
    uVar3 = __allmul(local_10.s.LowPart,local_10.s.HighPart,1000,0);
    local_4 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
    iVar2 = network_prepare_challenge_packet();
    if (iVar2 != 0) {
      local_11 = 0;
      if ((*(byte *)(iVar1 + 0xa8c) & 1) == 0) {
        FUN_004dcdb0(iVar1,iVar2,&local_11,1);
      }
      return 1;
    }
  }
  return 0;
}

Disassembly (objdump -d -M intel, bin/halo.exe) confirming network_prepare_challenge_packet's
real arguments:
  4e216c: lea edx,[esp+0x14]     ; edx = &{echoed_value, timestamp_ms}
  4e2170: mov eax,0x3            ; eax = message_type = 3
  4e2175: call 0x4deaf0
  4e217e: test BYTE PTR [esi+0xa8c],0x1
  4e2189: jne 0x4e21aa           ; flags bit0 set -> skip FUN_004dcdb0
#endif
```

## network_game_message_handle_ping_timestamp.c

```
// network_game_message_handle_ping_timestamp  (Ghidra: FUN_004e20b0; named per this rewrite)
// address 0x4e20b0, size 83 bytes
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Handles queued message type 0x34 by recording
// elapsed time since a stored timestamp into a field of the local player's datum." Called with
// a single explicit argument (`param_1`) from src/networking/network_client_drain_queued_updates.c's
// own `case 0x34: FUN_004e20b0(param_1);`, so `param_1` there and here are the same value; its
// offset +0x9c0 falls inside types/networking.h's network_server_globals::unknown_9bc[0x3c]
// span, accessed here the same way network_game_server_handle_client_join.c accesses the
// neighbouring +0x9c4 (an explicit byte-offset cast, since the header leaves that span
// unresolved).
// register convention: EAX = message (a pointer to the queued-message record pointer, matching
// network_client_drain_queued_updates.c's `&local_108`), stack = param_1 (server, UNSURE).
//   // blam-cc: EAX -> message, stack -> param_1
// UNSURE (major): FUN_004ec670, FUN_004ec590 and datum_get are all called with zero visible
// arguments in Ghidra's own decompile; FUN_004ec670/FUN_004ec590 are message-delta functions
// outside this batch's range (0x4ec2f0+) and datum_get's real signature elsewhere in this
// codebase takes (handle, array), neither of which is recoverable at this call site. Declared
// and called with no arguments here, matching Ghidra literally, rather than inventing a
// plausible index/array pair.
// UNSURE: FUN_00449210 (foreign, < this module) is presumed to return a millisecond-shaped
// tick count purely from how its result is subtracted from a stored timestamp.
```

```
#if 0
Original Ghidra decompilation (0x4e20b0), from tools/pack.py 0x4e20b0:

undefined4 FUN_004e20b0(int param_1)

{
  int iVar1;
  char cVar2;
  undefined4 *in_EAX;
  int iVar3;
  int iVar4;

  if (*(int *)*in_EAX != 0) {
    FUN_004ec670();
    return 1;
  }
  cVar2 = FUN_004ec590();
  if (cVar2 == '\x01') {
    iVar3 = datum_get();
    if (iVar3 != 0) {
      iVar1 = *(int *)(param_1 + 0x9c0);
      iVar4 = FUN_00449210();
      *(int *)(iVar3 + 0xdc) = iVar4 - iVar1;
    }
  }
  return 1;
}
#endif
```

## network_game_message_handle_player_count_broadcast.c

```
// network_game_message_handle_player_count_broadcast  (Ghidra: FUN_004e2530; named per this rewrite)
// address 0x4e2530, size 80 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md: "Handles message type 0x11 by decoding it and
// triggering a session-wide player-count broadcast" -- network_game_broadcast_player_set_changed
// (FUN_004e1bf0, this batch). Same decode shape as every sibling handler.
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> server, EDX -> buffer
// UNSURE: network_game_broadcast_player_set_changed is called here with zero visible arguments,
// matching Ghidra literally rather than its own file's (server, param_1) signature.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2530: ESI server, EDX record, stack length; state (+4) 0 or 1, class 3.
```

```
#if 0
Original Ghidra decompilation (0x4e2530), from tools/pack.py 0x4e2530:

undefined4 FUN_004e2530(void)

{
  char cVar1;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  if ((*(short *)(unaff_ESI + 4) == 0) || (*(short *)(unaff_ESI + 4) == 1)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_4,in_EDX + 2,local_8,
                       local_c,3);
    if (cVar1 != '\0') {
      FUN_004e1bf0();
    }
  }
  return 1;
}
#endif
```

## network_game_message_handle_player_entry_update.c

```
// network_game_message_handle_player_entry_update  (Ghidra: FUN_004e2580; named per this rewrite)
// address 0x4e2580, size 88 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md: "Handles message type 0x12, conditionally
// triggering a player-count broadcast after an approval check" -- the approval check is
// network_player_entry_update (FUN_004de5f0, already written), whose own signature is
// (network_player_entry *incoming, network_game_session *session); on success this broadcasts
// the player set change exactly like the type-0x11 handler.
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> server, EDX -> buffer
// UNSURE: network_player_entry_update and network_game_broadcast_player_set_changed are both
// called here with zero visible arguments, matching Ghidra literally rather than their own
// files' fuller signatures.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2580: ESI server, EDX record, stack length; state (+4) 0, class 3; the entry goes to
// network_player_entry_update (EAX entry, ECX server +8).
```

```
#if 0
Original Ghidra decompilation (0x4e2580), from tools/pack.py 0x4e2580:

undefined4 FUN_004e2580(void)

{
  char cVar1;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_28 [4];
  undefined1 local_24 [4];
  undefined1 local_20 [32];

  if (*(short *)(unaff_ESI + 4) == 0) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,local_24,
                       local_28,3);
    if (cVar1 != '\0') {
      cVar1 = FUN_004de5f0();
      if (cVar1 != '\0') {
        FUN_004e1bf0();
      }
    }
  }
  return 1;
}
#endif
```

## network_game_message_handle_retry_schedule.c

```
// network_game_message_handle_retry_schedule  (Ghidra: FUN_004e26a0; named per this rewrite)
// address 0x4e26a0, size 83 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.35)
// evidence: out/phase4/networking_functions.md's summary claims message type 0x15, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x14: case 0x25: FUN_004e26a0();` -- the low-confidence (0.3) summary swapped this
// function's type number with FUN_004e2630's; the dispatcher's literal call table is trusted
// here instead. Forwards to network_machine_timer_start, already written (blam-cc: ESI ->
// machine, stack -> duration_ms).
// register convention: EAX = server (implicit passthrough), ESI = machine (implicit
// passthrough, needed only for the network_machine_timer_start call), stack = buffer.
//   // blam-cc: EAX -> server, ESI -> machine, stack -> buffer
// UNSURE: `machine` is not read anywhere in this function's own body; its presence is inferred
// purely from network_machine_timer_start's own established ESI convention, following the same
// reasoning used throughout this batch for calls that pass a register-resident argument through
// unchanged.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e26a0: EAX server, ESI machine, stack (record, length); state (+4) 0, class 3; then the machine's
// timer starts (network_machine_timer_start: ESI machine, stack 0).
```

```
#if 0
Original Ghidra decompilation (0x4e26a0), from tools/pack.py 0x4e26a0:

undefined4 FUN_004e26a0(int param_1)

{
  char cVar1;
  int in_EAX;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  if (*(short *)(in_EAX + 4) == 0) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_4,param_1 + 2,local_8,
                       local_c,3);
    if (cVar1 != '\0') {
      FUN_004df090(0);
    }
  }
  return 1;
}
#endif
```

## network_game_message_handle_settings_relay.c

```
// network_game_message_handle_settings_relay  (Ghidra: FUN_004e24d0; named per this rewrite)
// address 0x4e24d0, size 86 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md: "Server-side handler for message type 0x10
// that decodes its payload and passes it to FUN_004df0e0" -- the already-written
// network_game_settings_broadcast_send. Follows the exact decode-then-forward shape shared by
// every sibling handler in this cluster (see network_game_process_incoming_message.c).
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough), matching every sibling handler in this batch.
//   // blam-cc: ESI -> server, EDX -> buffer
// UNSURE: network_game_settings_broadcast_send is called here with zero visible arguments in
// Ghidra's own decompile, against its own file's established (round, record) signature;
// declared and called with no arguments, matching Ghidra literally.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e24d0: ESI server, EDX record, stack length; state (+4) 0, class 3.
```

```
#if 0
Original Ghidra decompilation (0x4e24d0), from tools/pack.py 0x4e24d0:

undefined4 FUN_004e24d0(void)

{
  char cVar1;
  undefined4 uVar2;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_28 [4];
  undefined1 local_24 [4];
  undefined1 local_20 [32];

  if (*(short *)(unaff_ESI + 4) == 0) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,local_24,
                       local_28,3);
    if (cVar1 != '\0') {
      uVar2 = FUN_004df0e0();
      return uVar2;
    }
  }
  return 1;
}
#endif
```

## network_game_message_handle_settings_relay_role2.c

```
// network_game_message_handle_settings_relay_role2  (Ghidra: FUN_004e28d0; named per this rewrite)
// address 0x4e28d0, size 86 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md's summary claims message type 0x1e, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x23: FUN_004e28d0();` -- the dispatcher's literal call table is trusted here instead
// of the low-confidence (0.3) summary. This is the third of three role-gated forwarders to
// network_game_settings_broadcast_send (roles 0, 1 and 2 at message types 0x10, 0x1d and 0x23
// respectively), gated on role == 2 and decode class 7.
// register convention: ESI = context (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> context, EDX -> buffer
// UNSURE: network_game_settings_broadcast_send is called here with zero visible arguments;
// matching Ghidra literally.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e28d0: ESI server, EDX record, stack length; state (+4) 2, class 7.
```

```
#if 0
Original Ghidra decompilation (0x4e28d0), from tools/pack.py 0x4e28d0:

undefined4 FUN_004e28d0(void)

{
  char cVar1;
  undefined4 uVar2;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_28 [4];
  undefined1 local_24 [4];
  undefined1 local_20 [32];

  if (*(short *)(unaff_ESI + 4) == 2) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,local_24,
                       local_28,7);
    if (cVar1 != '\0') {
      uVar2 = FUN_004df0e0();
      return uVar2;
    }
  }
  return 1;
}
#endif
```

## network_game_server_handle_client_join.c

```
// network_game_server_handle_client_join  (Ghidra: FUN_004dfc90, unnamed)
// address 0x4dfc90, size 643 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Server/host-side handling of a client
// (re)join: validates the channel table, timestamps the join, and either delegates to the
// client path or spawns the new player's game objects and updates the owners..."
// param_1+0x1aa/+0x1c7/+0x3c4 match network_server_globals::session.players and
// ::machines[0].machine_id exactly as in the sibling ownership-handoff functions in this
// batch. The free-slot scan at DAT_006b0b88 matches types/game.h's player_profile_cache[16]
// (stride 0x30, in_use at +0x00, player datum at +0x04) and player_profile_cache_count
// (0x006f1d34) exactly.
// register convention: stack = server (network_server_globals *), machine (network_machine *).
// blam-cc: stack -> server, machine
// UNSURE: mirrors network_game_server_handoff_object_ownership.c's EDI (object-count)
// pass-through: this function calls FUN_004dfa10 with only its two visible arguments, so it
// must itself carry the same implicit EDI parameter, forwarded unchanged.
// UNSURE: the trailing `FUN_004dfc10()` call has no visible argument either; FUN_004dfc10's
// own parameter is BL (a slot index) per network_object_release_ownership_claim.c, so this
// function is modelled as also taking an implicit, forwarded-only BL byte. No source for that
// byte is visible in this function's own body.
// UNSURE: network_player_join_finalize, FUN_004de870, FUN_0045c440, FUN_00479f40 and
// datum_new_at_index_with_salt are called with no visible arguments at these sites; Ghidra's
// own signatures for them are likewise argument-less, so they are declared and called that
// way here, but their true register-passed parameters (if any) are not re-derived.
// UNSURE: the byte at network_machine+0x0e/0x0f is treated as a 16-bit read-modify-write in
// the original (`|= 4` on the word), but only bit 0x04 of the low byte (flags) actually
// changes; rewritten as `machine->flags |= 0x04`, which is byte-for-byte equivalent. Bit 0x04
// is not one of the enumerated network_machine_flags.
// UNSURE: server+0x9c4 has no named field (it falls inside network_server_globals's
// unresolved unknown_9bc span); accessed here through an explicit offset cast.
```

```
#if 0
Original Ghidra decompilation (0x4dfc90):

void FUN_004dfc90(int param_1,int param_2)

{
  undefined1 uVar1;
  char cVar2;
  char cVar3;
  short *psVar4;
  undefined4 uVar5;
  char *pcVar6;
  int iVar7;
  int iVar8;
  bool bVar9;
  undefined8 uVar10;
  LARGE_INTEGER local_8;

  *(ushort *)(param_2 + 0xe) = *(ushort *)(param_2 + 0xe) | 4;
  iVar7 = DAT_0071c2d8;
  bVar9 = true;
  if (*(short *)(param_1 + 4) != 1) {
    psVar4 = (short *)(param_1 + 0x3c4);
    iVar8 = 4;
    do {
      if (((-1 < *psVar4) && (*psVar4 < 0x10)) && ((*(byte *)(psVar4 + 1) & 4) == 0)) {
        bVar9 = false;
      }
      if (((-1 < psVar4[0x30]) && (psVar4[0x30] < 0x10)) && ((*(byte *)(psVar4 + 0x31) & 4) == 0)) {
        bVar9 = false;
      }
      if (((-1 < psVar4[0x60]) && (psVar4[0x60] < 0x10)) && ((*(byte *)(psVar4 + 0x61) & 4) == 0)) {
        bVar9 = false;
      }
      if (((-1 < psVar4[0x90]) && (psVar4[0x90] < 0x10)) && ((*(byte *)(psVar4 + 0x91) & 4) == 0)) {
        bVar9 = false;
      }
      psVar4 = psVar4 + 0xc0;
      iVar8 = iVar8 + -1;
    } while (iVar8 != 0);
    if (bVar9) {
      bVar9 = DAT_0071c2d8 == 0;
      *(undefined2 *)(param_1 + 4) = 1;
      *(undefined4 *)(param_1 + 0x9c4) = 0;
      if (bVar9) {
        uVar1 = 0;
      }
      else {
        uVar1 = *(undefined1 *)(iVar7 + 0xec0);
      }
      *(undefined1 *)(param_1 + 0x3b4) = uVar1;
    }
    if (*(int *)(param_1 + 0x9c4) == 0) {
      QueryPerformanceCounter(&local_8);
      uVar10 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
      uVar5 = __alldiv(uVar10,DAT_006ac8f8,DAT_006ac8fc);
      *(undefined4 *)(param_1 + 0x9c4) = uVar5;
    }
  }
  iVar7 = param_1 + 0x1aa;
  local_8.s.LowPart = 0x10;
LAB_004dfdb0:
  cVar2 = FUN_004de9f0();
  if (cVar2 != '\0') {
    iVar8 = 0;
    pcVar6 = (char *)(param_1 + 0x1c7);
    do {
      if ((pcVar6[-1] == *(char *)(iVar7 + 0x1c)) && (*pcVar6 == *(char *)(iVar7 + 0x1d))) {
        if ((short)*(char *)(iVar7 + 0x1c) == *(short *)(param_2 + 0xc)) {
          if ((*(byte *)(param_1 + 6) >> 2 & 1) == 0) {
            cVar2 = network_player_join_finalize();
          }
          else {
            cVar2 = '\0';
            cVar3 = FUN_004de9f0();
            if (((cVar3 != '\0') && (*(short *)(param_1 + 4) == 1)) &&
               (cVar2 = FUN_004de870(), cVar2 != '\0')) {
              FUN_004d98f0((int)*(char *)(iVar7 + 0x1f));
              datum_new_at_index_with_salt();
              datum_new_at_index_with_salt();
              FUN_00479f40();
            }
          }
          if (cVar2 != '\0') {
            *(undefined1 *)(param_2 + 0x50) = 1;
            uVar5 = FUN_004d98f0((int)*(char *)(iVar7 + 0x1f));
            FUN_0045c440(uVar5);
            iVar8 = FUN_00466e80();
            if (iVar8 != -1) goto LAB_004dfedb;
            iVar8 = 0;
            pcVar6 = (char *)&DAT_006b0b88;
            goto LAB_004dfeb0;
          }
        }
        break;
      }
      iVar8 = iVar8 + 1;
      pcVar6 = pcVar6 + 0x20;
    } while (iVar8 < 0x10);
  }
  goto LAB_004dfef9;
  while( true ) {
    pcVar6 = pcVar6 + 0x30;
    iVar8 = iVar8 + 1;
    if (0x6b0e87 < (int)pcVar6) break;
LAB_004dfeb0:
    if (*pcVar6 == '\0') {
      *(undefined1 *)(&DAT_006b0b88 + iVar8 * 0xc) = 1;
      (&DAT_006b0b8c)[iVar8 * 0xc] = uVar5;
      DAT_006f1d34 = DAT_006f1d34 + 1;
      break;
    }
  }
LAB_004dfedb:
  FUN_004dfa10(param_1,param_2);
  FUN_004dfc10();
LAB_004dfef9:
  iVar7 = iVar7 + 0x20;
  local_8.s.LowPart = local_8.s.LowPart - 1;
  if (local_8.s.LowPart == 0) {
    return;
  }
  goto LAB_004dfdb0;
}
#endif
```

## network_game_server_handle_info_request.c

```
// network_game_server_handle_info_request  (Ghidra: FUN_004e2700; named per this rewrite)
// address 0x4e2700, size 136 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md: "Handles message type 0x1a, sending a full
// server-info reply to not-yet-established machines or otherwise forwarding to FUN_004dfc90"
// -- network_game_server_handle_client_join, already written. `*unaff_EDI + 0xa98` matches
// network_channel::connected via network_machine::channel (offset 0), and
// `unaff_ESI + 0xa0f` matches network_server_globals::game_over, exactly as in the type-0xe and
// type-0xf handlers.
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough), EDI = machine (implicit passthrough).
//   // blam-cc: ESI -> server, EDX -> buffer, EDI -> machine
// UNSURE: both network_server_build_full_game_info_packet and
// network_game_server_handle_client_join are called here with zero visible arguments, against
// their own files' non-empty signatures; matching Ghidra literally.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2700: ESI server, EDI machine, EDX record, stack length; state (+4) 0 or 1, class 5. A machine whose
// connection (+0) has +0xa98 set, or any machine while the server's +0xa0f is clear, gets
// network_game_server_handle_client_join (stack server, machine) and 1; otherwise the full game info packet is
// built for it (0x4e0bd0) and its result returned. Returns 0 when the state or the decode fails.
```

```
#if 0
Original Ghidra decompilation (0x4e2700), from tools/pack.py 0x4e2700:

undefined4 FUN_004e2700(void)

{
  char cVar1;
  undefined4 uVar2;
  int in_EDX;
  int unaff_ESI;
  int *unaff_EDI;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  if ((*(short *)(unaff_ESI + 4) == 0) || (*(short *)(unaff_ESI + 4) == 1)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_4,in_EDX + 2,local_8,
                       local_c,5);
    if (cVar1 != '\0') {
      if ((((unaff_EDI == (int *)0x0) || (*unaff_EDI == 0)) ||
          (*(char *)(*unaff_EDI + 0xa98) == '\0')) && (*(char *)(unaff_ESI + 0xa0f) != '\0')) {
        uVar2 = FUN_004e0bd0();
        return uVar2;
      }
      FUN_004dfc90();
      return 1;
    }
  }
  return 0;
}
#endif
```

## network_game_server_handle_join_confirm.c

```
// network_game_server_handle_join_confirm  (Ghidra: FUN_004e2400)
// address 0x4e2400, size 200 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x4e2400..0x4e24c7: EAX machine, ECX server, EDX buffer, stack length: while
//   the host is not in a game, the decoded body is the player to add; failure sends reason 3, success broadcasts the
//   player set and sends a type 0xa accept. Returns 1.
// blam-cc: EAX -> machine, ECX -> server, EDX -> buffer, stack -> length
```

```
#if 0
Original Ghidra decompilation (0x4e2400), from tools/pack.py 0x4e2400:

undefined4 FUN_004e2400(void)

{
  char cVar1;
  int in_EAX;
  ushort *puVar2;
  int in_ECX;
  int in_EDX;
  undefined1 local_28 [4];
  undefined4 local_24;
  undefined1 local_20 [32];

  if ((*(short *)(in_ECX + 4) == 0) || (*(short *)(in_ECX + 4) == 1)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,&local_24,
                       local_28,3);
    if (cVar1 != '\0') {
      cVar1 = FUN_004df840();
      if (cVar1 == '\0') {
        FUN_004e0af0();
      }
      else {
        cVar1 = FUN_004e1bf0();
        if (cVar1 != '\0') {
          local_24 = 0;
          puVar2 = (ushort *)network_prepare_challenge_packet();
          if ((puVar2 != (ushort *)0x0) && (*(short *)(in_EAX + 0xc) != -1)) {
            network_session_send_to_machine(0,puVar2,(uint)(*puVar2 >> 4) << 3,1,0,1,3);
            return 1;
          }
        }
      }
    }
  }
  return 1;
}
#endif
```

## network_game_server_handle_join_password.c

```
// network_game_server_handle_join_password  (Ghidra: FUN_004e21d0)
// address 0x4e21d0, size 546 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x4e21d0..0x4e23f1: the host's join request handler (message 0xe; EBX machine,
//   stack server, buffer, length). While the host is not in a game (+4 0 or 1) and the machine is not already joining
//   (+0xe bit 1): a machine without a connected channel after the game ended gets the full game info. Otherwise the
//   request (a 0x84 byte body) is decoded; unless the server accepts joins (+6 bit 0, state 0 or 1) it is refused
//   (0); the CD key response at body +0x22 must pass the host check (else 6); the first 16 bytes must match the
//   version canary (else 1); the 8-character password at body +0x10 must match a set password (else 2); the machine
//   is reset and its player (body +0x6e) added (else 3), the player set is broadcast, the channel rate (+0xa88)
//   becomes 4 or body +0x6b by the hint byte 0x006894a2, and a type 0xa accept packet is sent. Refusals send the
//   reason; every path returns 1.
// blam-cc: EBX -> machine, stack -> server, buffer, length

// VERIFIED against disassembly 0x4e21d0..0x4e23f2 (2026-09-30): compared every branch (state/flags gate, full-game-info shortcut, decode args, reject reasons 0/6/1/2/3, add player, rate 4 or body+0x6b, type 0xa accept packet via send_to_machine 7 stack args); FIXED broadcast_player_set_changed takes only the session (one stack arg)
```

```
#if 0
Original Ghidra decompilation (0x4e21d0), from tools/pack.py 0x4e21d0:

char FUN_004e21d0(int param_1,int param_2)

{
  char cVar1;
  int *piVar2;
  ushort *puVar3;
  int iVar4;
  int *unaff_EBX;
  int *piVar5;
  bool bVar6;
  undefined4 local_b0;
  undefined1 local_ac [4];
  wchar_t local_a8 [12];
  int local_90 [4];
  wchar_t local_80 [8];
  undefined2 local_70;
  undefined1 local_6e [73];
  byte local_25;

  if (((*(short *)(param_1 + 4) == 0) || (*(short *)(param_1 + 4) == 1)) &&
     ((*(byte *)((int)unaff_EBX + 0xe) >> 1 & 1) == 0)) {
    if (((*unaff_EBX == 0) || (*(char *)(*unaff_EBX + 0xa98) == '\0')) &&
       (*(char *)(param_1 + 0xa0f) != '\0')) {
      cVar1 = FUN_004e0bd0();
      if (cVar1 != '\0') {
        return cVar1;
      }
    }
    else {
      cVar1 = data_packet_group_decode_packet
                        (&PTR_s_network_game_messages_group_006994f8,local_90,param_2 + 2,&local_b0,
                         local_ac,3);
      if (cVar1 != '\0') {
        FUN_004dd390();
        if ((((*(byte *)(param_1 + 6) & 1) != 0) &&
            ((*(short *)(param_1 + 4) == 0 || (*(short *)(param_1 + 4) == 1)))) &&
           (cVar1 = FUN_004e0ab0(local_6e), cVar1 != '\0')) {
          piVar2 = (int *)FUN_004e0790();
          iVar4 = 4;
          bVar6 = true;
          piVar5 = local_90;
          do {
            if (iVar4 == 0) break;
            iVar4 = iVar4 + -1;
            bVar6 = *piVar5 == *piVar2;
            piVar5 = piVar5 + 1;
            piVar2 = piVar2 + 1;
          } while (bVar6);
          if (bVar6) {
            FUN_004e0930();
            local_70 = 0;
            iVar4 = _wcsncmp(local_80,local_a8,8);
            if ((((iVar4 == 0) || (cVar1 = FUN_004e08e0(), cVar1 == '\0')) &&
                (cVar1 = FUN_004df690(), cVar1 != '\0')) && (cVar1 = FUN_004df840(), cVar1 != '\0'))
            {
              FUN_004e1bf0(param_1);
              if (DAT_006894a2 == '\0') {
                *(undefined4 *)(*unaff_EBX + 0xa88) = 4;
              }
              else {
                *(uint *)(*unaff_EBX + 0xa88) = (uint)local_25;
              }
              local_b0 = 0;
              puVar3 = (ushort *)network_prepare_challenge_packet();
              if (puVar3 == (ushort *)0x0) {
                return '\x01';
              }
              if ((short)unaff_EBX[3] == -1) {
                return '\x01';
              }
              network_session_send_to_machine(0,puVar3,(uint)(*puVar3 >> 4) << 3,1,0,1,3);
              return '\x01';
            }
          }
        }
        FUN_004e0af0(param_1);
      }
    }
  }
  return '\x01';
}

Disassembly (objdump -d -M intel, bin/halo.exe) recovering the reason-code dispatch Ghidra's own
pseudo-C collapsed into a single argument-less `FUN_004e0af0(param_1)`:
  4e2311: mov ecx,2 / mov edi,ebx / call 0x4e0af0    ; reason 2: password configured and mismatched
                                                        (falls through from the password_is_set
                                                        check when wcsncmp also mismatched)
  4e23c9: push ebp / mov ecx,3 / jmp shared-call      ; reason 3: network_machine_reset or
                                                        network_game_session_finalize_and_add_player failed
  4e23d4: push ebp / mov ecx,1 / jmp shared-call      ; reason 1: canary/challenge memcmp mismatch
  4e23df: push ebp / mov ecx,6 / jmp shared-call      ; reason 6: network_join_request_reset_state failed
  4e23ea: push ebp / xor ecx,ecx / jmp shared-call    ; reason 0: session flags/state not ready
#endif
```

## network_game_server_handoff_object_ownership.c

```
// network_game_server_handoff_object_ownership  (Ghidra: FUN_004dfa10, unnamed)
// address 0x4dfa10, size 512 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Attempts to hand off ownership of the
// object identified by param_2 to a matching channel slot in param_1's table, invoking a
// completion callback stored at DAT_006f1d20+0x90 once all 16 slots have been [processed]."
// param_1+0x1aa is network_server_globals::session.players (session at +0x008, players at
// session+0x1a2). param_1+0x3c4 is network_server_globals::machines[0].machine_id
// (machines at +0x3b8, machine_id at +0xc). player+0x20/+0x34 match types/game.h player's
// team and unit fields; object_header/object 0x106 match types/objects.h's vitality_flags
// and _object_health_frozen_bit.
// register convention: stack = server (network_server_globals *), machine (network_machine *).
// blam-cc: stack -> server, machine
// UNSURE: this function calls FUN_004df950 twice with no register set up for FUN_004df950's
// own EDI (object-count) parameter, meaning EDI must be an implicit pass-through parameter of
// THIS function too; modelled here as an extra, forwarded-only int32_t * parameter,
// register EDI, whose value this function never itself reads.
// UNSURE: when the inner machine-id search (over network_machine::machine_id) fails to find
// a match, the original leaves `iVar5` at 0 and falls straight into
// `*(byte *)(iVar5 + 0xe)`, i.e. a read of absolute address 0xe. Preserved exactly (no bounds
// check invented); this path is presumably unreachable in practice since a valid player
// entry's machine_index should always resolve to a live machine.
// UNSURE: FUN_004d98f0's parameter convention (a single byte in AL, here the entry's
// slot_index) is inferred only from this and the sibling call in
// network_object_release_ownership_claim.c.
// UNSURE: FUN_004779d0, FUN_00466e80, FUN_00466ee0, FUN_00477a80, build_player_full_resync_update
// signatures are inferred solely from their arguments at this call site.
// reconciled: R04 0x006f1d20 void * network_game_engine_callback_block -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)
```

```
#if 0
Original Ghidra decompilation (0x4dfa10):

void FUN_004dfa10(int param_1,int param_2)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  short sVar4;
  int iVar5;
  short *psVar6;
  short sVar7;
  int iVar8;
  int iVar9;
  int local_10;
  int local_c;
  undefined4 local_8;
  undefined4 local_4;

  local_10 = 0;
  local_c = 0;
  iVar8 = (int)*(short *)(param_2 + 0xc);
  FUN_004df950(param_1,&local_10);
  FUN_004df950(param_1,&local_10);
  iVar9 = param_1 + 0x1aa;
  local_c = 0x10;
LAB_004dfa70:
  local_10 = iVar9;
  cVar1 = FUN_004de9f0();
  if (cVar1 != '\0') {
    cVar1 = *(char *)(iVar9 + 0x1c);
    iVar5 = 0;
    pcVar2 = (char *)(param_1 + 0x1c7);
    do {
      if ((pcVar2[-1] == cVar1) && (*pcVar2 == *(char *)(iVar9 + 0x1d))) {
        if ((short)cVar1 != *(short *)(param_2 + 0xc)) {
          iVar5 = 0;
          iVar3 = 0;
          psVar6 = (short *)(param_1 + 0x3c4);
          goto LAB_004dfac6;
        }
        break;
      }
      iVar5 = iVar5 + 1;
      pcVar2 = pcVar2 + 0x20;
    } while (iVar5 < 0x10);
  }
  goto LAB_004dfbd8;
  while( true ) {
    iVar3 = iVar3 + 1;
    psVar6 = psVar6 + 0x30;
    if (0xf < iVar3) break;
LAB_004dfac6:
    if (*psVar6 == (short)cVar1) {
      iVar5 = iVar3 * 0x60 + 0x3b8 + param_1;
      break;
    }
  }
  if (((((*(byte *)(iVar5 + 0xe) & 4) != 0) &&
       (iVar5 = FUN_004d98f0((int)*(char *)(iVar9 + 0x1f)), iVar5 != -1)) &&
      (sVar7 = (short)iVar5, -1 < sVar7)) && (sVar7 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar3 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar7;
    sVar7 = *(short *)(iVar3 + *(int *)(DAT_0087a480 + 0x34));
    iVar3 = iVar3 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar7 != 0) && ((sVar4 = (short)((uint)iVar5 >> 0x10), sVar4 == 0 || (sVar7 == sVar4)))) {
      local_8 = *(undefined4 *)(iVar3 + 0x20);
      local_4 = *(undefined4 *)(iVar3 + 0x34);
      FUN_004779d0(local_8);
      build_player_full_resync_update(iVar8);
      iVar9 = FUN_00466e80();
      if (iVar9 != -1) {
        FUN_00466ee0(0);
      }
      iVar9 = local_10;
      if ((*(uint *)(iVar3 + 0x34) != 0xffffffff) &&
         ((*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                             (*(uint *)(iVar3 + 0x34) & 0xffff) * 0xc) + 0x106) & 4) == 0)) {
        FUN_00477a80(iVar5,local_8,iVar8);
        iVar9 = local_10;
      }
    }
  }
LAB_004dfbd8:
  iVar9 = iVar9 + 0x20;
  local_c = local_c + -1;
  if (local_c == 0) {
    if (*(code **)(DAT_006f1d20 + 0x90) != (code *)0x0) {
      local_10 = iVar9;
      (**(code **)(DAT_006f1d20 + 0x90))(0,iVar8);
    }
    return;
  }
  goto LAB_004dfa70;
}
#endif
```

## network_game_server_host_create.c

```
// network_game_server_host_create  (Ghidra: network_game_server_host_create, already named)
// address 0x4ddd40, size 73 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Allocates and installs the network host globals
// (DAT_0071c2d4) via network_game_server_host_new and seeds its randomisation salt field from the global PRNG
// state, mirroring the salt into the network-game globals when present." network_server
// (0x0071c2d4) and network_client (0x0071c2d8) match types/networking.h.
// UNSURE: the salt field this function writes (host+0x3ac, i.e. session-relative +0x3a4) lands
// two bytes into types/networking.h's network_game_session.unknown_3a2[10] and would spill into
// unknown_3ac/pad_3ad for a 4-byte write; the header does not carve out a dedicated dword there,
// so this rewrite keeps the raw offset rather than asserting a field boundary the header doesn't
// support.
```

```
#if 0
Original Ghidra decompilation (0x4ddd40):

int __cdecl network_game_server_host_create(void)

{
  int iVar1;
  void *pvVar2;
  uint uVar3;
  bool bVar4;

  pvVar2 = network_game_server_host_new();
  iVar1 = DAT_0071c2d8;
  DAT_0071c2d4 = pvVar2;
  if (pvVar2 != (void *)0x0) {
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    uVar3 = DAT_00719cd4 >> 0x10;
    bVar4 = DAT_0071c2d8 != 0;
    *(uint *)((int)pvVar2 + 0x3ac) = uVar3;
    if (bVar4) {
      *(uint *)(iVar1 + 0xeb8) = uVar3;
    }
  }
  return CONCAT31((int3)((uint)pvVar2 >> 8),pvVar2 != (void *)0x0);
}
#endif
```

## network_game_server_host_dispose.c

```
// network_game_server_host_dispose  (Ghidra: network_game_server_host_dispose, already named)
// address 0x4deda0, size 284 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Tears down the network host globals block
// passed in param_1: disposes each of its 16 team/player sub-entries, frees its allocation,
// zeroes the structure, and shuts down the associated transport connection." host->flags bit2
// (+6, stats logging), machines[16] at +0x3b8 stride 0x60 with channel/machine_id/flags, and the
// final 0x284-dword (0xa10-byte) zero of the whole network_server_globals all match
// types/networking.h exactly.
// UNSURE: network_channel_service's channel argument (network_channel_service's own `channel` parameter) is
// elided at this call site; reconstructed as the current machine's channel, matching the loop's
// own subject.
// register convention: fully recovered cdecl (host is the only parameter).
```

```
#if 0
Original Ghidra decompilation (0x4deda0):

void __cdecl network_game_server_host_dispose(int *param_1)

{
  int iVar1;
  int *piVar2;

  FUN_0061b3f0(DAT_0069fdfc);
  if ((*(byte *)((int)param_1 + 6) >> 2 & 1) != 0) {
    message_delta_parameters_protocol_dump_to_config_file();
    network_stats_summary_log_write();
  }
  if ((((short)param_1[1] == 0) || ((short)param_1[1] == 2)) &&
     (iVar1 = network_prepare_challenge_packet(), iVar1 != 0)) {
    FUN_004e19c0(0,iVar1,1,0,1,3);
  }
  piVar2 = param_1 + 0xee;
  iVar1 = 0x10;
  do {
    if (((short)piVar2[3] != -1) && ((~(byte)(*(uint *)(*piVar2 + 0xa8c) >> 4) & 1) != 0)) {
      FUN_004dd110(0);
    }
    piVar2 = piVar2 + 0x18;
    iVar1 = iVar1 + -1;
  } while (iVar1 != 0);
  if ((int *)*param_1 != (int *)0x0) {
    network_channel_delete((int *)*param_1);
  }
  for (iVar1 = 0x284; iVar1 != 0; iVar1 = iVar1 + -1) {
    *param_1 = 0;
    param_1 = param_1 + 1;
  }
  DAT_0071c2ec = 0;
  if (DAT_00722a20 != 0) {
    if (DAT_00722a18 != 2) {
      DAT_00722a18 = 2;
    }
    FUN_00577940();
    DAT_0069fdfc = 0xffffffff;
    FUN_0061b760();
    FUN_00616c40(DAT_00722a20);
    DAT_00722a20 = 0;
  }
  return;
}
#endif
```

## network_game_server_host_new.c

```
// network_game_server_host_new  (Ghidra: network_game_server_host_new, already named)
// address 0x4dec40, size 338 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: types/networking.h's own comment cites this address: "network_game_server_host_new
// (0x4dec40) zeroes 0x284 dwords of 0x00861340, which is the size of network_server_globals."
// Every field this function sets after the zero (flags bit1, session.message_callback,
// session.unknown_19e, and the 16-entry machines[] init matching network_machine's
// channel/unknown_04/unknown_08/machine_id/flags/unknown_50/unknown_51/unknown_52/unknown_56/
// unknown_5c fields, including the header's own "unaligned in the original" note on
// unknown_52/unknown_56) matches types/networking.h exactly.
// UNSURE: the tail zeroing (host+0x9c4..0x9d4, +0x9f8/+0x9f9/+0x9fa) only partly lines up with
// the header's "unknown_9bc[0x3c]" catch-all array; kept as raw offsets into that array rather
// than asserting finer field boundaries the header doesn't declare.
```

```
#if 0
Original Ghidra decompilation (0x4dec40):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __cdecl network_game_server_host_new(void)

{
  char cVar1;
  undefined4 *puVar2;
  int iVar3;
  undefined4 *puVar4;

  puVar4 = &DAT_00861340;
  puVar2 = &DAT_00861340;
  for (iVar3 = 0x284; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  DAT_0071c2ec = 1;
  DAT_00699f44 = 0;
  DAT_0071cc24 = 0;
  DAT_00861d4e = 0;
  DAT_00861d4f = 0;
  network_channels_open();
  DAT_00861340 = network_channel_new(1);
  if (DAT_00861340 != (int *)0x0) {
    DAT_00861344._2_1_ = DAT_00861344._2_1_ | 2;
    DAT_00861344._0_2_ = 0;
    puVar2 = &DAT_00861348;
    for (iVar3 = 0xec; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    DAT_00861340[2] = (int)&LAB_004e1410;
    network_channel_table_initialize();
    _DAT_008614e6 = DAT_00696564;
    DAT_008616f0 = -1;
    puVar2 = &DAT_008616fc;
    do {
      puVar2[-1] = 0;
      *puVar2 = 0;
      puVar2[1] = 0;
      *(undefined2 *)(puVar2 + 2) = 0xffff;
      *(undefined2 *)((int)puVar2 + 10) = 0;
      *(undefined4 *)((int)puVar2 + 0x4e) = 0;
      *(undefined4 *)((int)puVar2 + 0x52) = 0;
      puVar2[0x16] = 0xffffffff;
      *(undefined1 *)(puVar2 + 0x13) = 0;
      *(undefined1 *)((int)puVar2 + 0x4d) = 0;
      puVar2 = puVar2 + 0x18;
    } while ((int)puVar2 < 0x861cfc);
    _DAT_00861d08 = 0;
    _DAT_00861d0c = 0;
    _DAT_00861d10 = 0;
    _DAT_00861cf8 = 0;
    _DAT_00861d04 = 0;
    DAT_00861d39 = 0;
    DAT_00861d3a = 0;
    DAT_00861d38 = 0;
    DAT_008616f0 = DAT_008616f0 + 1;
    _DAT_00861d14 = 0;
    cVar1 = FUN_004e1820();
    if (cVar1 != '\0') {
      FUN_00577850(0);
      goto LAB_004ded77;
    }
  }
  network_game_server_host_dispose((int *)&DAT_00861340);
  puVar4 = (undefined4 *)0x0;
LAB_004ded77:
  if ((*(byte *)((int)puVar4 + 6) >> 2 & 1) != 0) {
    message_delta_protocol_initialize();
    network_stats_summary_log_open();
  }
  return puVar4;
}
#endif
```

## network_game_server_load_scenario.c

```
// network_game_server_load_scenario  (Ghidra: FUN_004e0720, unnamed)
// address 0x4e0720, size 106 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Loads the requested scenario for the host
// (via FUN_004de6d0), resetting per-round counters first, and optionally logs the operation
// to the debug log file when verbose host logging is enabled." DAT_0071c2d4 is
// types/networking.h's `network_server`; +6 is ::flags (stats-logging bit, same test as in
// other functions in this batch); DAT_0087ac06/006f14b4/006a6140/00719879 are all named
// globals from that header (debug_log_level, network_statistics_logging_enabled,
// network_summary_log_file, network_build_string).
// register convention: none; every operand is a fixed global.
// UNSURE: DAT_00699f44 and DAT_0071cc24 have no established names elsewhere in this module;
// declared here as generic per-round counters reset before every scenario load.
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)
```

```
#if 0
Original Ghidra decompilation (0x4e0720):

char FUN_004e0720(void)

{
  int iVar1;
  char cVar2;

  iVar1 = DAT_0071c2d4;
  DAT_00699f44 = 0;
  DAT_0071cc24 = 0;
  cVar2 = network_game_scenario_load_request(DAT_0071c2d4 + 8);
  if (((((*(byte *)(iVar1 + 6) >> 2 & 1) != 0) && (2 < DAT_0087ac06)) && (DAT_006f14b4 != '\0')) &&
     (DAT_006a6140 != (FILE *)0x0)) {
    _fprintf(DAT_006a6140,"%s\t",&DAT_00719879);
  }
  return cVar2;
}
#endif
```

## network_game_server_per_frame_tick.c

```
// network_game_server_per_frame_tick  (Ghidra: FUN_004e03c0, unnamed)
// address 0x4e03c0, size 183 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Per-frame network tick: in client-processing
// state, drains the queued update packets (FUN_00472cc0) and applies the first matching
// channel's queued update via FUN_004df840/FUN_004e1b50; in the alternate[, ticks the game
// engine directly]." unaff_ESI+4 matches network_server_globals::unknown_004;
// unaff_ESI+0x3c4/+0x9f8 match ::machines[0].machine_id and ::unknown_9f8.
// register convention: CX = update_count (int16_t), ESI = server (network_server_globals *).
// blam-cc: CX -> update_count, ESI -> server
// The byte read at server+0x9f4 is pending_join_entry.machine_index (raw offset cast kept).
// `if (machine != 0)` mirrors the original's (always-true) test at 0x4e0448.

// VERIFIED against disassembly 0x4e03c0..0x4e0477 (2026-09-30): FIXED: no EAX argument (EAX is overwritten by movzx at 0x4e03c0); the finalize/broadcast entry is &server->pending_join_entry (esi+0x9d8, EAX for both callees); broadcast_state_snapshot(EAX = entry, stack = server) was called without args. State dispatch, update loop, machine search (stride 0x60, machine_id word vs sign-extended byte 0x9f4) and pending flag clear match
```

```
#if 0
Original Ghidra decompilation (0x4e03c0):

void FUN_004e03c0(void)

{
  char cVar1;
  int iVar2;
  ushort in_CX;
  short *psVar3;
  int unaff_ESI;
  uint uVar4;
  LARGE_INTEGER local_c;

  if (*(short *)(unaff_ESI + 4) == 1) {
    if (0 < (short)in_CX) {
      uVar4 = (uint)in_CX;
      do {
        *(int *)(unaff_ESI + 0x9b8) = *(int *)(unaff_ESI + 0x9b8) + 1;
        FUN_00472cc0();
        QueryPerformanceCounter(&local_c);
        uVar4 = uVar4 - 1;
      } while (uVar4 != 0);
    }
    if (*(char *)(unaff_ESI + 0x9f8) != '\0') {
      iVar2 = 0;
      psVar3 = (short *)(unaff_ESI + 0x3c4);
      while (*psVar3 != (short)*(char *)(unaff_ESI + 0x9f4)) {
        iVar2 = iVar2 + 1;
        psVar3 = psVar3 + 0x30;
        if (0xf < iVar2) {
          *(undefined1 *)(unaff_ESI + 0x9f8) = 0;
          return;
        }
      }
      if (iVar2 * 0x60 + 0x3b8 + unaff_ESI != 0) {
        cVar1 = FUN_004df840();
        if (cVar1 != '\0') {
          FUN_004e1b50();
        }
      }
      *(undefined1 *)(unaff_ESI + 0x9f8) = 0;
    }
  }
  else if (*(short *)(unaff_ESI + 4) == 2) {
    game_engine_tick();
    return;
  }
  return;
}
#endif
```

## network_game_server_send_message_to_all_machines_ingame.c

```
// network_game_server_send_message_to_all_machines_ingame  (Ghidra: already named)
// address 0x4e4ef0, size 12 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md ("Clears a flag on a broadcast-message context,
// likely restricting a subsequent server broadcast to only clients currently in-game").
// register convention: the sole instruction operates on an implicit ESI pointer Ghidra could not
// attribute to a formal parameter; no caller in this batch or elsewhere in the codebase was
// found to cross-check the pointed-to type against.
//   // blam-cc: ESI -> context
// UNSURE: the shape/owner of `context`; +0x1c is written as a single byte with no other evidence
// in this batch of what type it belongs to.
```

```
#if 0
Original Ghidra decompilation (0x4e4ef0), from tools/pack.py 0x4e4ef0:

void network_game_server_send_message_to_all_machines_ingame(void)

{
  int unaff_ESI;

  *(undefined1 *)(unaff_ESI + 0x1c) = 0;
  return;
}
#endif
```

## network_game_session_finalize_and_add_player.c

```
// network_game_session_finalize_and_add_player  (Ghidra: FUN_004df840, unnamed)
// address 0x4df840, size 185 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Sanitises and finalises a newly-joining
// player's name and colour (rejecting reserved characters, resolving collisions), then
// registers the player in the channel table via FUN_004de4e0." The reserved characters are
// literally '%' and '|' (the `local_8` scratch), matching the join-name escaping used
// elsewhere in this module. Field reads against `entry` (in_EAX) match
// types/networking.h's network_player_entry exactly: word 0xe -> byte 0x1c
// (machine_index, sign-extended), word 0xf -> byte 0x1e (unknown_1e), word 0xc -> 0x18
// (color_index). `in_ECX + 8` lands on network_server_globals::session (offset 0x008).
// register convention: EAX = entry (network_player_entry *), ECX = server
// (network_server_globals *), EDX = machine (network_machine *).
//   // blam-cc: EAX -> entry, ECX -> server, EDX -> machine
// FIXED (register inputs, objdump): EDX carries machine (read at 0x4df850,
// cmp WORD PTR [edx+0xc],cx); the prose note above already named it correctly but had no
// machine-readable "// blam-cc:" line and its own continuation wrapped without the file's usual
// two-space "//" indent, so the checker's parser cut the note off before reaching EDX.
// UNSURE: `in_EDX` is inferred to be the joining machine's network_machine record purely
// from `*(short *)(in_EDX + 0xc)` matching machine_id's offset; no caller in this batch
// shows the argument being loaded, so the pointer's exact source is not re-derived here.
// UNSURE: the final `return (uint)in_EAX & 0xffffff00;` on the mismatch path is transcribed
// literally; it looks like leftover register reuse from the compiler rather than a
// meaningful result, but semantics are preserved as-is.
```

```
#if 0
Original Ghidra decompilation (0x4df840):

uint FUN_004df840(void)

{
  undefined1 uVar1;
  char cVar2;
  wchar_t *in_EAX;
  wchar_t *pwVar3;
  uint uVar4;
  int in_ECX;
  int in_EDX;
  wchar_t local_8 [4];

  if (*(short *)(in_EDX + 0xc) == (short)(char)in_EAX[0xe]) {
    local_8[0] = L'%';
    local_8[1] = L'\0';
    local_8[2] = L'|';
    local_8[3] = L'\0';
    if ((char)in_EAX[0xf] == -1) {
      uVar1 = FUN_00470720(0xffffffff);
      *(undefined1 *)(in_EAX + 0xf) = uVar1;
    }
    if (*in_EAX == L'\0') {
      network_game_generate_unique_random_name();
    }
    pwVar3 = _wcsstr(in_EAX,local_8);
    if ((pwVar3 != (wchar_t *)0x0) ||
       (pwVar3 = _wcsstr(in_EAX,local_8 + 2), pwVar3 != (wchar_t *)0x0)) {
      network_game_generate_unique_random_name();
    }
    cVar2 = FUN_004df6f0();
    if (cVar2 == '\0') {
      network_game_generate_unique_random_name();
    }
    if (in_EAX[0xc] == L'\xffff') {
      FUN_004df790();
    }
    uVar4 = FUN_004de4e0(in_ECX + 8);
    return uVar4;
  }
  return (uint)in_EAX & 0xffffff00;
}
#endif
```

## network_game_session_reset_defaults.c

```
// network_game_session_reset_defaults  (Ghidra: FUN_004e1820, unnamed)
// address 0x4e1820, size 82 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md's network_game_session section: "FUN_004e1820
// copies 0x26 dwords (0x98 bytes) of 0x0087aa80 (game.h's game_engine_pending_variant) to
// server + 0x10c, i.e. session+0x104 ... The same function does
// strncpy(server + 0x8c, 0x0087aa40, 0x3f) ... and zeroes word session+0x07e and dword
// session+0x080." listen_channel->listening (channel+0xae0) and server->flags bit0 match
// types/networking.h exactly.
// register convention: EBX = server (network_server_globals *).
// blam-cc: EBX -> server
```

```
#if 0
Original Ghidra decompilation (0x4e1820):

void FUN_004e1820(void)

{
  int iVar1;
  int *unaff_EBX;
  int *piVar2;
  int *piVar3;

  piVar2 = &DAT_0087aa80;
  piVar3 = unaff_EBX + 0x43;
  for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
    *piVar3 = *piVar2;
    piVar2 = piVar2 + 1;
    piVar3 = piVar3 + 1;
  }
  _strncpy((char *)(unaff_EBX + 0x23),&DAT_0087aa40,0x3f);
  *(undefined1 *)((int)unaff_EBX + 0xcb) = 0;
  *(undefined2 *)((int)unaff_EBX + 0x86) = 0;
  unaff_EBX[0x22] = 0;
  *(ushort *)((int)unaff_EBX + 6) = *(ushort *)((int)unaff_EBX + 6) | 1;
  *(undefined1 *)(*unaff_EBX + 0xae0) = 1;
  return;
}
#endif
```

## network_host_full_state_broadcast.c

```
// network_host_full_state_broadcast  (Ghidra: FUN_004df510; named per this rewrite)
// address 0x4df510, size 302 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "The first time it runs for a round, sends a
// type-0x21 packet to every channel entry flagged as needing a full state refresh, staggering
// each send's embedded timestamp by 100ms." host->unknown_a0e (the "run once" latch, cleared
// here) and host->game_over (+0xa0f, cleared here) match types/networking.h; the machines[]
// iteration (stride 0x60, byte offset +0x3c6 == machines[0]+0xe == flags) matches
// network_game_client_game_settings_updated.c's own machines[] loop over the same field.
// UNSURE: bit4 (0x10) of machine->flags is not named in network_machine_flags; kept as a literal
// bit test. data_packet_group_encode_packet is called with a minimal, call-site-local prototype
// (see network_send_join_request_packet.c's precedent), not the fuller src/memory
// reconstruction.
// register convention: fully recovered cdecl (host is the only parameter).
```

```
#if 0
Original Ghidra decompilation (0x4df510):

/* WARNING: Type propagation algorithm not settling */

void FUN_004df510(int param_1)

{
  char cVar1;
  uint uVar2;
  undefined2 *puVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  int local_614;
  int local_610;
  uint local_60c [3];
  undefined4 local_600 [384];

  if (*(char *)(param_1 + 0xa0e) == '\x01') {
    *(undefined1 *)(param_1 + 0xa0e) = 0;
    *(undefined1 *)(param_1 + 0xa0f) = 0;
    local_614 = 1000;
    puVar3 = (undefined2 *)(param_1 + 0x3c6);
    local_610 = 0x10;
    do {
      if ((((byte)*puVar3 >> 1 & 1) != 0) || (((byte)*puVar3 >> 4 & 1) != 0)) {
        local_60c[1] = 0;
        local_60c[2] = local_614;
        local_60c[0] = 0x600;
        cVar1 = data_packet_group_encode_packet(local_60c + 1,local_60c,0x21,1);
        if (cVar1 != '\0') {
          DAT_006b7f98 = ((short)local_60c[0] + 2) * 0x10 | 0xc;
          puVar4 = local_600;
          puVar5 = &DAT_006b7f9a;
          for (uVar2 = (local_60c[0] & 0xffff) >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
            *puVar5 = *puVar4;
            puVar4 = puVar4 + 1;
            puVar5 = puVar5 + 1;
          }
          for (uVar2 = local_60c[0] & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
            *(undefined1 *)puVar5 = *(undefined1 *)puVar4;
            puVar4 = (undefined4 *)((int)puVar4 + 1);
            puVar5 = (undefined4 *)((int)puVar5 + 1);
          }
          cVar1 = network_session_send_to_machine
                            (0,&DAT_006b7f98,(uint)(DAT_006b7f98 >> 4) << 3,1,0,0,3);
          if (cVar1 != '\0') {
            local_614 = local_614 + 100;
          }
        }
      }
      puVar3 = puVar3 + 0x30;
      local_610 = local_610 + -1;
    } while (local_610 != 0);
  }
  return;
}
#endif
```

## network_host_round_reset.c

```
// network_host_round_reset  (Ghidra: FUN_004df640; named per this rewrite)
// address 0x4df640, size 65 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Resets the per-round update counters and
// completion flags on the object at in_EAX, increments its round counter, and calls network_game_session_reset_defaults
// to continue setup." Same host offsets (+0x9b8, +0x9bc-region, +0x3b0) as
// network_game_server_host_new.c; see that file's header for the field-matching evidence and the
// same "+0x3b0 lands in session.unknown_3a2[10]" UNSURE note.
// register convention: host in EAX (in_EAX). blam-cc: EAX -> host
```

```
#if 0
Original Ghidra decompilation (0x4df640):

void FUN_004df640(void)

{
  int in_EAX;

  *(undefined4 *)(in_EAX + 0x9c8) = 0;
  *(undefined4 *)(in_EAX + 0x9cc) = 0;
  *(undefined4 *)(in_EAX + 0x9d0) = 0;
  *(undefined4 *)(in_EAX + 0x9d4) = 0;
  *(undefined4 *)(in_EAX + 0x9b8) = 0;
  *(undefined4 *)(in_EAX + 0x9c4) = 0;
  *(undefined1 *)(in_EAX + 0x9f9) = 0;
  *(undefined1 *)(in_EAX + 0x9fa) = 0;
  *(undefined1 *)(in_EAX + 0x9f8) = 0;
  *(int *)(in_EAX + 0x3b0) = *(int *)(in_EAX + 0x3b0) + 1;
  FUN_004e1820();
  return;
}
#endif
```

## network_host_send_scenario_announcement.c

```
// network_host_send_scenario_announcement  (Ghidra: FUN_004df1c0; named per this rewrite)
// address 0x4df1c0, size 204 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md: "Sends the one-time scenario/challenge
// announcement packets (via FUN_004ec940/FUN_004e19c0 and network_prepare_challenge_packet) the first time it is
// called for this game, then latches a done flag." host->unknown_9f9/unknown_9b8 match
// network_game_server_host_new.c's established offsets on network_server_globals.
// UNSURE: message_delta_encode_message's parameter shapes are inferred purely from this call
// site; declared generically.
// register convention: host in ESI (unaff_ESI). blam-cc: ESI -> host
```

```
#if 0
Original Ghidra decompilation (0x4df1c0):

bool FUN_004df1c0(void)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  int unaff_ESI;
  void *local_c;
  undefined4 local_8;
  undefined4 local_4;

  bVar1 = true;
  if (*(char *)(unaff_ESI + 0x9f9) == '\0') {
    local_8 = 0;
    message_delta_parameters_protocol_send_update();
    local_c = (void *)(unaff_ESI + 8);
    local_4 = 0;
    iVar3 = message_delta_encode_message(0,0x21,0,&local_c,0,1,'\0');
    if (0 < iVar3) {
      FUN_004e19c0(1,&DAT_00871de0,1,0,1,3);
    }
    bVar1 = 0 < iVar3;
    if (0 < iVar3) {
      iVar3 = network_prepare_challenge_packet();
      if (iVar3 != 0) {
        cVar2 = FUN_004e19c0(0,iVar3,1,0,1,3);
        if (cVar2 != '\0') {
          *(undefined1 *)(unaff_ESI + 0x9f9) = 1;
          bVar1 = true;
        }
      }
    }
  }
  *(undefined4 *)(unaff_ESI + 0x9b8) = 0;
  return bVar1;
}
#endif
```

## network_host_shutdown_or_defer.c

```
// network_host_shutdown_or_defer  (Ghidra: FUN_004ddd90; named per this rewrite)
// address 0x4ddd90, size 184 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "If a network host is active and not in the
// special team-sync case, resets the host's map-load flag and history state and disposes the
// host globals; otherwise defers to network_host_update_tick." session->unknown_3ac (the map-loaded flag)
// matches types/networking.h's network_game_session exactly when reached through
// network_server->session or network_client->session directly (unlike
// network_game_server_host_create.c's host-relative offset, which lands elsewhere -- see that
// file's UNSURE note).
// UNSURE: DAT_0071c2dd, DAT_00719754 and DAT_0071973c are not documented anywhere in
// types/networking.h and are not touched by any other file in this batch; declared generically
// below and left unrenamed.
```

```
#if 0
Original Ghidra decompilation (0x4ddd90):

undefined4 FUN_004ddd90(void)

{
  int *piVar1;
  undefined4 uVar2;

  if (DAT_0071c2d4 != (int *)0x0) {
    if ((DAT_0071c2de != '\x01') || ((*(byte *)((int)DAT_0071c2d4 + 6) >> 2 & 1) == 0)) {
      uVar2 = FUN_004def80();
      return uVar2;
    }
    DAT_00719720 = 0;
    main_menu_music_stop();
    if (DAT_0071c2d4 == (int *)0x0) {
      if (DAT_0071c2d8 == 0) {
        piVar1 = (int *)0x0;
      }
      else {
        piVar1 = (int *)(DAT_0071c2d8 + 0xb14);
      }
    }
    else {
      piVar1 = DAT_0071c2d4 + 2;
    }
    if ((char)piVar1[0xeb] != '\0') {
      chimera__load_ui_map('\x01');
    }
    piVar1[0xeb] = 0;
    FUN_004dde70();
    if (DAT_0071c2d4 != (int *)0x0) {
      network_game_server_host_dispose(DAT_0071c2d4);
      DAT_0071c2d4 = (int *)0x0;
      DAT_0071c2dd = 0;
    }
    DAT_00719754._0_2_ = 0xffff;
    DAT_0071973c = 0;
    DAT_00719754._3_1_ = 1;
  }
  return 1;
}
#endif
```

## network_host_update_tick.c

```
// network_host_update_tick  (Ghidra: FUN_004def80; named per this rewrite)
// address 0x4def80, size 239 bytes
// name confidence: 0.3   rewrite confidence: 0.15
// evidence: out/phase4/networking_functions.md: "Per-tick client-side network update handler:
// pulls the next queued packet, refreshes the map/variant cycle list periodically, and
// dispatches processing to one of three state-specific handlers based on [state]." host->flags
// bit1 (k_network_server_host, matches network_game_server_host_new.c) and host->unknown_004
// (the state dispatched on 0/1/2, matching network_game_server_host_dispose.c's own 0/2 test)
// match types/networking.h.
// FIXED in the review pass (this was the file's largest UNSURE): `network_channel_service(local_20)` is
// network_channel_service's real third, stack-passed argument, which Ghidra dropped from that
// function's own signature. 0x4def97..0x4defa8 is
//     mov edi,[esi]                 ; channel   = host->listen_channel
//     lea edx,[esp+0xc] / push edx  ; out_new_child = &new_child
//     xor eax,eax                   ; timeout_ms = 0
//     mov [esp+0x10],0              ; new_child = NULL before the call
// and 0x4defbb reads the local straight back. So the listening service hands the newly
// accepted child channel back through that pointer, and the rest of this function is the
// accept path for it. See src/networking/network_channel_service.c.
// FIXED: 0x4defc3 pushes the new child as network_server_count_machines_and_resolve_address's second argument, and 0x4defe1 pushes
// it as network_channel_remove_child's `child` (with EDI reloaded to host->listen_channel);
// the first draft passed 0 for both.
```

```
#if 0
Original Ghidra decompilation (0x4def80):

char FUN_004def80(void)

{
  short sVar1;
  int iVar2;
  char cVar3;
  char cVar4;
  int in_EAX;
  uint uVar5;
  int local_20 [8];

  cVar3 = '\x01';
  if ((*(byte *)(in_EAX + 6) >> 1 & 1) != 0) {
    local_20[0] = 0;
    cVar3 = FUN_004dd110(local_20);
    iVar2 = local_20[0];
    if (cVar3 == '\x01') {
      cVar4 = '\x01';
      if (local_20[0] != 0) {
        cVar3 = FUN_004e0d30();
        if (cVar3 == '\x01') {
          FUN_004dd390();
          cVar4 = '\x01';
        }
        else {
          cVar4 = FUN_004dd090(iVar2);
        }
      }
      cVar3 = '\0';
      if (cVar4 != '\0') {
        uVar5 = FUN_00449210();
        if (*(int *)(in_EAX + 0x9c0) + 3000U < uVar5) {
          FUN_004deec0();
          *(uint *)(in_EAX + 0x9c0) = uVar5;
        }
        cVar3 = FUN_004e11d0();
        if (cVar3 == '\0') {
          return '\0';
        }
        sVar1 = *(short *)(in_EAX + 4);
        if (sVar1 == 0) {
          cVar3 = FUN_004e15a0();
          return cVar3;
        }
        if (sVar1 != 1) {
          if (sVar1 != 2) {
            return '\0';
          }
          cVar3 = FUN_004e1450();
          return cVar3;
        }
        cVar3 = FUN_004e1520();
        return cVar3;
      }
    }
  }
  return cVar3;
}
#endif
```

## network_join_request_reset_state.c

```
// network_join_request_reset_state  (Ghidra: FUN_004e0ab0)
// address 0x4e0ab0, size 63 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x4e0ab0..0x4e0aee: EAX machine, stack response: the CD key check of a joining
//   machine with its remote ip (the local address 0x006869b0 for loopback 127.0.0.1), its challenge (+0x52) and CD
//   key local id (+0x5c). (Name kept.)
// blam-cc: EAX -> machine, stack -> response
```

```
#if 0
Original Ghidra decompilation (0x4e0ab0):

void FUN_004e0ab0(void)

{
  FUN_004dd390();
  FUN_00575ff0();
  return;
}
#endif
```

## network_machine_check_build_version.c

```
// network_machine_check_build_version  (Ghidra: FUN_004dff20, unnamed)
// address 0x4dff20, size 72 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Compares the version/build string at
// DAT_00719879 against the caller-supplied string and flags the object at unaff_EDI as
// mismatched (bit 3 of +0xe) if they differ." 0x00719879 is types/networking.h's
// network_build_string; +0xe matches network_machine::flags,
// k_network_machine_version_mismatch = 0x08.
// register convention: EAX = remote_version (const char *), EDI = machine (network_machine *).
// blam-cc: EAX -> remote_version, EDI -> machine
// UNSURE: the hand-rolled byte loop computes a full three-way comparison result that this
// function only ever tests for zero; rewritten as a plain equality scan, which is
// byte-for-byte equivalent for that purpose.
// UNSURE (important): traced literally, the flag is set when the two strings are EQUAL
// (`iVar3 == 0`, reached only by matching all the way to the terminating NUL), not when they
// differ -- the opposite polarity of both the functions.md summary and of what the existing
// k_network_machine_version_mismatch name suggests. Reusing that enumerator here anyway
// (types/*.h is not edited by this batch) but the polarity below is the literal one from the
// decompilation, not the summary's.
```

```
#if 0
Original Ghidra decompilation (0x4dff20):

void FUN_004dff20(void)

{
  byte bVar1;
  byte *in_EAX;
  byte *pbVar2;
  int iVar3;
  int unaff_EDI;
  bool bVar4;

  pbVar2 = &DAT_00719879;
  do {
    bVar1 = *pbVar2;
    bVar4 = bVar1 < *in_EAX;
    if (bVar1 != *in_EAX) {
LAB_004dff58:
      iVar3 = (1 - (uint)bVar4) - (uint)(bVar4 != 0);
      goto LAB_004dff5d;
    }
    if (bVar1 == 0) break;
    bVar1 = pbVar2[1];
    bVar4 = bVar1 < in_EAX[1];
    if (bVar1 != in_EAX[1]) goto LAB_004dff58;
    pbVar2 = pbVar2 + 2;
    in_EAX = in_EAX + 2;
  } while (bVar1 != 0);
  iVar3 = 0;
LAB_004dff5d:
  if (iVar3 == 0) {
    *(byte *)(unaff_EDI + 0xe) = *(byte *)(unaff_EDI + 0xe) | 8;
  }
  return;
}
#endif
```

## network_machine_clear_flag_by_id.c

```
// network_machine_clear_flag_by_id  (Ghidra: FUN_004e0b90, unnamed)
// address 0x4e0b90, size 63 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Finds the machine-table slot matching a given
// id and clears a flag/name byte at offset 0x50 within that slot." Matches
// network_machine::unknown_50 exactly.
// register convention: EDX = server (network_server_globals *), EDI = machine_id (int32_t).
// blam-cc: EDX -> server, EDI -> machine_id
// UNSURE: the low byte of the returned pointer is masked off (`& 0xffffff00`), matching the
// same odd idiom seen in network_game_session_finalize_and_add_player.c; preserved literally.
// UNSURE: when no match is found, the original writes a zero byte to the fixed absolute
// address 0x50 (`DAT_00000050 = 0`) rather than through any parameter -- almost certainly a
// decompilation artifact (a register Ghidra treated as a bare small-integer global), but
// transcribed exactly as shown per the no-invented-behaviour rule.
```

```
#if 0
Original Ghidra decompilation (0x4e0b90):

uint FUN_004e0b90(void)

{
  uint uVar1;
  short *psVar2;
  int in_EDX;
  int unaff_EDI;

  uVar1 = 0;
  psVar2 = (short *)(in_EDX + 0x3c4);
  do {
    if (*psVar2 == unaff_EDI) {
      uVar1 = uVar1 * 0x60 + 0x3b8 + in_EDX;
      *(undefined1 *)(uVar1 + 0x50) = 0;
      return uVar1 & 0xffffff00;
    }
    uVar1 = uVar1 + 1;
    psVar2 = psVar2 + 0x30;
  } while ((int)uVar1 < 0x10);
  DAT_00000050 = 0;
  return uVar1 & 0xffffff00;
}
#endif
```

## network_machine_find_by_id.c

```
// network_machine_find_by_id  (Ghidra: FUN_004e0810, unnamed)
// address 0x4e0810, size 46 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: out/phase4/networking_types_notes.md "network_machine": "0x4e0810 walks
// machine_id from server+0x3c4 with a stride of 0x60 for 16 iterations and returns
// server + 0x3b8 + i*0x60, which pins both the base and the stride."
// register convention: ESI = server (network_server_globals *), EDI = machine_id (int32_t).
// blam-cc: ESI -> server, EDI -> machine_id
```

```
#if 0
Original Ghidra decompilation (0x4e0810):

int FUN_004e0810(void)

{
  int iVar1;
  short *psVar2;
  int unaff_ESI;
  int unaff_EDI;

  iVar1 = 0;
  psVar2 = (short *)(unaff_ESI + 0x3c4);
  do {
    if (*psVar2 == unaff_EDI) {
      return iVar1 * 0x60 + 0x3b8 + unaff_ESI;
    }
    iVar1 = iVar1 + 1;
    psVar2 = psVar2 + 0x30;
  } while (iVar1 < 0x10);
  return 0;
}
#endif
```

## network_machine_reset.c

```
// network_machine_reset  (Ghidra: FUN_004df690; named per this rewrite)
// address 0x4df690, size 82 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: types/networking.h cites this address directly: "network_machine (0x4dec40 init,
// 0x4df690 reset, ...)". flags |= k_network_machine_pending, timer_14/timer_18/unknown_50, and
// the 0xd-dword (0x34-byte) zero of connect_state[0x34] at +0x1c all match exactly.
// The leading network_channel_remote_address_or_default call takes EAX = machine->channel and
// ECX = &local scratch; its result is unused (verified 0x4df6a8..0x4df6b9).
// register convention: machine in ESI (unaff_ESI). blam-cc: ESI -> machine

// VERIFIED against disassembly 0x4df690..0x4df6e2 (2026-09-30)
```

```
#if 0
Original Ghidra decompilation (0x4df690):

undefined4 FUN_004df690(void)

{
  int iVar1;
  int unaff_ESI;
  undefined4 *puVar2;

  FUN_004dd390();
  *(byte *)(unaff_ESI + 0xe) = *(byte *)(unaff_ESI + 0xe) | 2;
  *(undefined1 *)(unaff_ESI + 0x10) = 0;
  *(undefined4 *)(unaff_ESI + 0x14) = 0;
  *(undefined4 *)(unaff_ESI + 0x18) = 0;
  *(undefined1 *)(unaff_ESI + 0x50) = 0;
  puVar2 = (undefined4 *)(unaff_ESI + 0x1c);
  for (iVar1 = 0xd; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  return 1;
}
#endif
```

## network_machine_timer_start.c

```
// network_machine_timer_start  (Ghidra: FUN_004df090; named per this rewrite)
// address 0x4df090, size 75 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Starts a timer on the object at unaff_ESI:
// records the current time, marks it active, and computes its expiry as now plus the given
// duration." Fields +0x10/+0x14/+0x18 match network_machine's unknown_10/timer_14/timer_18
// exactly (same object network_machine_reset.c clears).
// register convention: machine in ESI (unaff_ESI). blam-cc: ESI -> machine, stack -> duration_ms
```

```
#if 0
Original Ghidra decompilation (0x4df090):

void FUN_004df090(int param_1)

{
  int iVar1;
  int unaff_ESI;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  iVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  *(int *)(unaff_ESI + 0x14) = iVar1;
  *(undefined1 *)(unaff_ESI + 0x10) = 1;
  *(int *)(unaff_ESI + 0x18) = iVar1 + param_1;
  return;
}
#endif
```

## network_object_record_last_sender.c

```
// network_object_record_last_sender  (Ghidra: FUN_004df900, unnamed)
// address 0x4df900, size 75 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Records the current sender (unaff_ESI)
// into the per-machine last-sender table for the machine resolved from param_1, when that
// machine is a real network machine and the object is flagged accordingly." `param_1 + 0x3b4`
// is exactly network_server_globals::session (offset 0x008) + network_game_session's
// unknown_3ac (0x3ac), i.e. server + 0x3b4. The write target
// `(index & 0xffff) * 0x200 + 0xd0 + player_data->data` matches types/game.h's player
// (stride 0x200) field unknown_d0 at offset 0xd0.
// register convention: ESI = sender (int32_t datum/object handle), EAX = step_count (forwarded
// to player_data_iterator_advance), stack = server (network_server_globals *).
// blam-cc: EAX -> step_count, ESI -> sender, stack -> server
// FIXED (register inputs, objdump): EAX carries step_count (pushed at 0x4df901, right before
// `call 0x4d98f0`); it is the argument player_data_iterator_advance.c's own header flagged as
// "not visible at this call site" -- it is visible, just passed in through EAX rather than
// constructed locally, and this function never writes eax before that push.
```

```
#if 0
Original Ghidra decompilation (0x4df900):

undefined4 FUN_004df900(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  int unaff_ESI;

  uVar1 = FUN_004d98f0();
  if (uVar1 == 0xffffffff) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
    if (((*(char *)(param_1 + 0x3b4) != '\0') && (uVar1 != 0)) && (unaff_ESI != -1)) {
      *(int *)((uVar1 & 0xffff) * 0x200 + 0xd0 + *(int *)(DAT_0087a480 + 0x34)) = unaff_ESI;
      return uVar2;
    }
  }
  return uVar2;
}
#endif
```

## network_server_advance_connect_state.c

```
// network_server_advance_connect_state  (Ghidra: FUN_004df290; renamed by the review pass
// from network_machine_advance_connect_state -- see the register-convention note below)
// address 0x4df290, size 72 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Advances the connection state from 1 to 2 and
// flushes any pending challenge packet built by network_prepare_challenge_packet."
// REVIEW PASS 2026-09-20: the ESI argument is the SERVER, not a machine. The binary passes it
// straight through as network_session_broadcast_to_all ECX (0x4df2cc `mov ecx,esi`), and that
// callee immediately computes `esi = ecx + 0x3b8`, which is network_server_globals::machines --
// so ECX, and therefore this function ESI, is a network_server_globals *. The state word this
// function flips is at server+0x04.
// Also: the binary pushes only SIX stack arguments to 0x4e19c0 (0x4df2bb..0x4df2ca) and passes
// the encoded size in EAX (0x4df2b8..0x4df2c7, `(packet[0] >> 4) << 3`). Ghidra renders that
// register value as a seventh stack argument; it is dropped here. See src/networking/README.md,
// open question 2, for the wider EAX-argument gap on 0x4e19c0.
// UNSURE: the state word is read/written at server+4; accessed here as a 16-bit value via cast,
// matching Ghidra own `short` view.
// register convention: server in ESI (unaff_ESI). blam-cc: ESI -> server
```

```
#if 0
Original Ghidra decompilation (0x4df290):

void FUN_004df290(void)

{
  int iVar1;
  int unaff_ESI;
  undefined4 uVar2;

  if (*(short *)(unaff_ESI + 4) == 1) {
    uVar2 = 0;
    *(undefined2 *)(unaff_ESI + 4) = 2;
    iVar1 = network_prepare_challenge_packet();
    if (iVar1 != 0) {
      FUN_004e19c0(0,iVar1,1,0,1,3,uVar2);
    }
  }
  return;
}
#endif
```

## network_server_any_machine_awaiting_flag.c

```
// network_server_any_machine_awaiting_flag  (Ghidra: FUN_004e14e0, unnamed)
// address 0x4e14e0, size 53 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Reports whether any machine slot is still
// awaiting a particular connection flag, used to gate further handshake work." Traced
// literally: returns 1 as soon as every one of the 16 slots is either not connected (id out of
// 0..15) or has k_network_machine_version_mismatch set, and returns 0 the moment it finds a
// connected, non-mismatched slot.
// register convention: ECX = server (network_server_globals *).
// blam-cc: ECX -> server
```

```
#if 0
Original Ghidra decompilation (0x4e14e0):

undefined1 FUN_004e14e0(void)

{
  int in_ECX;
  short *psVar1;
  int iVar2;

  iVar2 = 0;
  psVar1 = (short *)(in_ECX + 0x3c4);
  while (((*psVar1 < 0 || (0xf < *psVar1)) || ((*(byte *)(psVar1 + 1) >> 3 & 1) != 0))) {
    iVar2 = iVar2 + 1;
    psVar1 = psVar1 + 0x30;
    if (0xf < iVar2) {
      return 1;
    }
  }
  return 0;
}
#endif
```

## network_server_build_full_game_info_packet.c

```
// network_server_build_full_game_info_packet  (Ghidra: FUN_004e0bd0, unnamed)
// address 0x4e0bd0, size 281 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Builds and queues a larger 'full game info'
// packet (message type 7) for a connecting machine, marking the request as answered."
// param_1[0]/+0xe match network_machine::channel/flags; the encode+flush sequence mirrors
// network_server_build_game_info_packet.c's (message group 4 there, group 7 here), confirming
// channel+0xa98/+0xa8c/+0x24/+0x1c/+0x20/+0xa80/+0x2c as ::connected/::flags/the outgoing
// bit_stream's last_bit/byte_cursor/bit_cursor/::send_budget/::outgoing.empty.
// register convention: stack = machine (network_machine *).
// blam-cc: stack -> machine
// UNSURE: the `channel->connected != 1` gate is the literal condition (not `== 1`); when the
// channel *is* already connected, this function reports success without sending anything,
// which is the polarity actually written rather than what "connected" suggests -- preserved
// as-is, as with the similar oddities elsewhere in this batch.
// UNSURE: bit 0x10 of network_machine::flags (the "answered" mark) is not one of the
// enumerated network_machine_flags values.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from encoded_buffer; the C passed placeholders or dropped the arguments.
```

```
#if 0
Original Ghidra decompilation (0x4e0bd0):

char FUN_004e0bd0(int *param_1)

{
  int iVar1;
  int iVar2;
  char cVar3;
  char cVar4;
  int iVar5;
  ushort *local_608;
  undefined1 local_604 [1540];

  local_608 = (ushort *)0x600;
  cVar3 = data_packet_group_encode_packet(local_604,&local_608,7,1);
  if (cVar3 == '\0') {
    return '\0';
  }
  local_608 = (ushort *)FUN_00440350(local_608);
  if (local_608 == (ushort *)0x0) {
    return '\0';
  }
  iVar5 = (uint)(*local_608 >> 4) * 8;
  cVar3 = '\0';
  if ((((param_1 != (int *)0x0) && (iVar2 = *param_1, iVar2 != 0)) &&
      (*(char *)(iVar2 + 0xa98) != '\x01')) && (iVar2 != 0)) {
    iVar1 = iVar5 + 1;
    if ((*(byte *)(iVar2 + 0xa8c) & 1) == 0) {
      if (((*(int *)(iVar2 + 0x24) + *(int *)(iVar2 + 0x1c) * -8) - *(int *)(iVar2 + 0x20)) + 1 <
          iVar1) {
        cVar4 = FUN_004ddb60(iVar2,1);
        cVar3 = '\0';
        if (cVar4 == '\0') goto LAB_004e0ccb;
      }
      *(int *)(iVar2 + 0xa80) = *(int *)(iVar2 + 0xa80) + iVar1;
      FUN_004cf8f0(1);
      *(undefined1 *)(iVar2 + 0x2c) = 0;
      FUN_004cf8f0(iVar5);
      *(undefined1 *)(iVar2 + 0x2c) = 0;
      cVar3 = FUN_004ddb60(iVar2,1);
    }
    else {
      cVar3 = '\x01';
    }
  }
LAB_004e0ccb:
  if (cVar3 == '\0') {
    return '\0';
  }
  *(byte *)((int)param_1 + 0xe) = *(byte *)((int)param_1 + 0xe) | 0x10;
  return cVar3;
}
#endif
```

## network_server_build_game_info_packet.c

```
// network_server_build_game_info_packet  (Ghidra: FUN_004e0950, unnamed)
// address 0x4e0950, size 345 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Builds and queues a small 'game info' style
// packet (short name plus a game-data snapshot) for the channel referenced by param_2, encoded
// as message type 4." param_2+0x0/+0xc match network_machine::channel/machine_id;
// param_1+0x88 (server + 0x88 == session + 0x80) copies exactly 0x84 bytes -- session's
// unknown_080, server_name[64] and unknown_0c4[0x40] back to back -- into a scratch snapshot.
// The free-space/flush sequence on the result matches network_channel::outgoing
// (bit_stream last_bit/byte_cursor/bit_cursor at +0x24/+0x1c/+0x20) and
// network_channel_stream_flush's own documented (stream, channel, mode) call shape.
// register convention: stack = server (network_server_globals *), machine (network_machine *).
// blam-cc: stack -> server, machine
// UNSURE: param_2's +0x52..+0x59 region is declared in types/networking.h as two separate
// "unaligned int32" fields (unknown_52, unknown_56) on network_machine; this function's
// 7-character-plus-NUL strncpy into exactly that 8-byte span strongly suggests it is really a
// `char short_name[8]`, but the header is not edited here -- accessed through a raw offset
// instead, with this note standing in for a TYPES-GAP.
// UNSURE: Ghidra's individual locals (local_6a6, local_6a4, local_6a0[7], local_699, local_698,
// local_694, local_690[419]) are contiguous on the stack in exactly that order (each one's
// ebp-relative offset abuts the next), so `local_6a0` -- the sole pointer passed to
// data_packet_group_encode_packet -- is really the address of one combined record: 7-byte
// short name, NUL, a flag byte, 3 bytes of padding, a machine-id dword, then the 0x84-byte
// snapshot, with roughly 1.5KB of additional scratch after it (up to the 0x600-byte size cap
// passed alongside). Modelled here as one struct-shaped local instead of Ghidra's separate
// variables.
// UNSURE: FUN_00575fa0 (source of the machine's short name copied at param_2+0x52 -- wait, see
// below), DAT_006894a2, FUN_004cf8f0 and the exact meaning of the packed word returned by
// network_message_block_build (`(*result >> 4) * 8` as a bit length) are not independently confirmed.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from encoded_buffer; the C passed placeholders or dropped the arguments.
```

```
#if 0
Original Ghidra decompilation (0x4e0950):

char FUN_004e0950(int param_1,int *param_2)

{
  int iVar1;
  char cVar2;
  char *_Source;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  size_t _Count;
  char local_6a6;
  ushort *local_6a4;
  char local_6a0 [7];
  undefined1 local_699;
  undefined1 local_698;
  int local_694;
  undefined4 local_690 [419];

  _Count = 7;
  _Source = (char *)FUN_00575fa0();
  _strncpy((char *)((int)param_2 + 0x52),_Source,_Count);
  *(undefined1 *)((int)param_2 + 0x59) = 0;
  iVar1 = param_2[3];
  _strncpy(local_6a0,(char *)((int)param_2 + 0x52),7);
  local_699 = 0;
  puVar4 = (undefined4 *)(param_1 + 0x88);
  puVar5 = local_690;
  for (iVar3 = 0x21; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  local_698 = DAT_006894a2;
  local_6a4 = (ushort *)0x600;
  local_694 = (int)(short)iVar1;
  cVar2 = data_packet_group_encode_packet(local_6a0,&local_6a4,4,1);
  if ((cVar2 != '\0') && (local_6a4 = (ushort *)FUN_00440350(local_6a4), local_6a4 != (ushort *)0x0)
     ) {
    iVar1 = *param_2;
    iVar3 = (uint)(*local_6a4 >> 4) * 8;
    if (iVar1 != 0) {
      local_6a6 = '\x01';
      if ((*(byte *)(iVar1 + 0xa8c) & 1) == 0) {
        if ((((*(int *)(iVar1 + 0x24) + *(int *)(iVar1 + 0x1c) * -8) - *(int *)(iVar1 + 0x20)) + 1 <
             iVar3 + 1) && (local_6a6 = FUN_004ddb60(iVar1,1), local_6a6 == '\0')) {
          return '\0';
        }
        *(int *)(iVar1 + 0xa80) = *(int *)(iVar1 + 0xa80) + iVar3 + 1;
        FUN_004cf8f0(1);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
        FUN_004cf8f0(iVar3);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
      }
      return local_6a6;
    }
  }
  return '\0';
}
#endif
```

## network_server_check_machine_timeout.c

```
// network_server_check_machine_timeout  (Ghidra: FUN_004e0ef0, unnamed)
// address 0x4e0ef0, size 727 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Per-machine per-tick check that detects a
// stale/timed-out connection, tears down its machine-table slot, and broadcasts a
// player-left style notification." machine+0x14/+0x18 match network_machine::timer_14/timer_18;
// machine+0x1c matches the start of ::connect_state (read here as a raw 0x20-byte copy of the
// *player* entry it is being compared against, not the machine's own connect_state -- see the
// UNSURE note below); +0x3b8/+0x3bc/+0x408/+0x409/+0x40a/+0x40e/+0x414 (relative to a computed
// `server + i*0x60` base) resolve, once `+0x3b8` is folded back in, to exactly
// machines[i]'s channel/unknown_04/unknown_50/unknown_51/unknown_52/unknown_56/unknown_5c.
// register convention: both parameters are genuine stack (cdecl) parameters per Ghidra's own
// signature.
// blam-cc: stack -> server, machine
// REVIEW PASS 2026-09-20: the stack-copy ambiguity is resolved against the disassembly
// (objdump -d -M intel --start-address=0x4e0ef0 --stop-address=0x4e11d0 bin/halo.exe). The two
// loops in this function genuinely differ and both readings are now transcribed as written:
//   - the unknown_50 == 0 loop copies the 0x20-byte network_player_entry to the stack first and
//     then passes the COPY to all three calls (0x4e102b `lea eax,[esp+0x20]` before 0x4de9f0,
//     0x4e1049 `lea edx,[esp+0x20]` pushed to 0x4df0e0, 0x4e105b / 0x4e1086 `lea eax,[esp+0x20]`
//     before both 0x4de640 calls). An earlier rewrite modelled these as receiving the live entry;
//     they do not.
//   - the unknown_50 != 0 loop has no copy and passes the LIVE entry (0x4e0f80 `mov eax,esi`,
//     0x4e0f9a `push esi`).
// The odd `network_client != -0xb14` test is real: the binary computes
// `lea ebx,[eax+0xb14] ; test ebx,ebx` at 0x4e106d, i.e. it null-checks a pointer 0xb14 bytes
// into the client globals rather than the base.
// UNSURE: FUN_004de640 is called a second time, conditionally, immediately after the first
// call already removed the same entry -- transcribed exactly as decompiled even though this
// looks redundant.
// UNSURE: DAT_0069fdfc ("the rcon/console connection id") and gcd_disconnect_user/gcd_disconnect_all are
// GameSpy-adjacent library calls with no further-established signatures here.
// UNSURE: the wraparound-aware timeout test (`uVar2 <= uVar1`, etc., on timer_14/timer_18
// against the current millisecond clock) is preserved exactly as decompiled without
// simplification, since its edge-case behaviour around clock wraparound is not safe to guess.
```

```
#if 0
Original Ghidra decompilation (0x4e0ef0):

undefined4 FUN_004e0ef0(int param_1,void *param_2)

{
  uint uVar1;
  uint uVar2;
  bool bVar3;
  char cVar4;
  uint uVar5;
  void *pvVar6;
  undefined4 *puVar7;
  int iVar8;
  undefined4 *puVar9;
  int iVar10;
  undefined4 *puVar11;
  undefined8 uVar12;
  int local_30;
  LARGE_INTEGER local_28;
  undefined4 local_20 [7];
  char local_4;

  QueryPerformanceCounter(&local_28);
  uVar12 = __allmul(local_28.s.LowPart,local_28.s.HighPart,1000,0);
  uVar5 = __alldiv(uVar12,DAT_006ac8f8,DAT_006ac8fc);
  pvVar6 = param_2;
  uVar1 = *(uint *)((int)param_2 + 0x18);
  uVar2 = *(uint *)((int)param_2 + 0x14);
  local_30 = 0;
  if (uVar2 <= uVar1) {
    if ((uVar1 <= uVar5) || (uVar5 < uVar2)) goto LAB_004e0f5e;
    if (uVar2 <= uVar1) {
      return 1;
    }
  }
  if ((uVar5 < uVar1) || (uVar2 <= uVar5)) {
    return 1;
  }
LAB_004e0f5e:
  if (*(char *)((int)param_2 + 0x50) == '\0') {
    local_28.s.LowPart = (DWORD)*(short *)((int)param_2 + 0xc);
    if (local_28.s.LowPart != 0xffffffff) {
      puVar7 = (undefined4 *)(param_1 + 0x1aa);
      local_30 = 0x10;
      do {
        puVar9 = puVar7;
        puVar11 = local_20;
        for (iVar8 = 8; iVar8 != 0; iVar8 = iVar8 + -1) {
          *puVar11 = *puVar9;
          puVar9 = puVar9 + 1;
          puVar11 = puVar11 + 1;
        }
        cVar4 = FUN_004de9f0();
        if (((((cVar4 != '\0') && ((int)local_4 == local_28.s.LowPart)) &&
             (cVar4 = FUN_004df0e0(param_1,local_20), cVar4 != '\0')) &&
            ((FUN_004de640(), DAT_0071c2d8 != 0 && (DAT_0071c2d8 != -0xb14)))) &&
           ((*(byte *)((int)param_2 + 0xe) >> 2 & 1) != 0)) {
          FUN_004de640();
        }
        puVar7 = puVar7 + 8;
        local_30 = local_30 + -1;
      } while (local_30 != 0);
      iVar8 = 0;
      pvVar6 = (void *)(param_1 + 0x3b8);
      do {
        if (pvVar6 == param_2) {
          iVar10 = iVar8 * 0x60 + param_1;
          if (*(int *)(iVar10 + 0x3b8) != 0) {
            FUN_004dd090(*(int *)(iVar10 + 0x3b8));
          }
          *(undefined4 *)(iVar10 + 0x3b8) = 0;
          *(undefined4 *)(iVar10 + 0x3bc) = 0;
          *(undefined4 *)((iVar8 * 3 + 0x1e) * 0x20 + param_1) = 0;
          *(undefined2 *)(iVar10 + 0x3c4) = 0xffff;
          *(undefined2 *)(iVar10 + 0x3c6) = 0;
          *(undefined4 *)(iVar10 + 0x40a) = 0;
          *(undefined4 *)(iVar10 + 0x40e) = 0;
          if (*(int *)(iVar10 + 0x414) == -1) {
            FUN_0061b3f0(DAT_0069fdfc);
          }
          else {
            FUN_0061b350(DAT_0069fdfc,*(int *)(iVar10 + 0x414));
          }
          *(undefined1 *)(iVar10 + 0x408) = 0;
          *(undefined1 *)(iVar10 + 0x409) = 0;
          *(undefined4 *)(iVar10 + 0x414) = 0xffffffff;
          message_delta_parameters_protocol_send_update();
          local_28.s.LowPart = 0;
          param_2 = (void *)(param_1 + 8);
          iVar8 = message_delta_encode_message(0,0x21,0,&param_2,0,1,'\0');
          if (0 < iVar8) {
            FUN_004e19c0(1,&DAT_00871de0,1,0,1,3);
          }
          return 2;
        }
        iVar8 = iVar8 + 1;
        pvVar6 = (void *)((int)pvVar6 + 0x60);
      } while (iVar8 < 0x10);
    }
    return 0;
  }
  if (*(char *)((int)param_2 + 0x51) != '\0') {
    return 1;
  }
  bVar3 = false;
  iVar8 = param_1 + 0x1aa;
  iVar10 = 0;
  do {
    cVar4 = FUN_004de9f0();
    if ((cVar4 != '\0') && ((short)*(char *)(iVar8 + 0x1c) == *(short *)((int)pvVar6 + 0xc))) {
      cVar4 = FUN_004df0e0(param_1,iVar8);
      if (cVar4 == '\0') {
        local_30 = 0;
        break;
      }
      bVar3 = true;
      local_30 = 1;
    }
    iVar10 = iVar10 + 1;
    iVar8 = iVar8 + 0x20;
  } while (iVar10 < 0x10);
  if (!bVar3) {
    local_30 = 1;
  }
  *(undefined1 *)((int)pvVar6 + 0x51) = 1;
  return local_30;
}
#endif
```

## network_server_count_connected_machines.c

```
// network_server_count_connected_machines  (Ghidra: FUN_004e1880, unnamed)
// address 0x4e1880, size 38 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md's network_machine section: "FUN_004e1880
// counts slots where *(int *)(id - 0xc) != 0 && id != -1, naming +0x00 as the channel
// pointer."
// register convention: ECX = server (network_server_globals *).
// blam-cc: ECX -> server
```

```
#if 0
Original Ghidra decompilation (0x4e1880):

int FUN_004e1880(void)

{
  int iVar1;
  int in_ECX;
  short *psVar2;
  int iVar3;

  iVar1 = 0;
  psVar2 = (short *)(in_ECX + 0x3c4);
  iVar3 = 0x10;
  do {
    if ((*(int *)(psVar2 + -6) != 0) && (*psVar2 != -1)) {
      iVar1 = iVar1 + 1;
    }
    psVar2 = psVar2 + 0x30;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  return iVar1;
}
#endif
```

## network_server_count_machines_and_resolve_address.c

```
// network_server_count_machines_and_resolve_address  (Ghidra: FUN_004e0d30, unnamed)
// address 0x4e0d30, size 438 bytes
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Counts occupied machine-table slots up to the
// first free one and, for a new connection, fetches its remote address via
// network_channel_get_remote_address." Ghidra's own decompilation removes eleven blocks as
// "unreachable", so most of this 438-byte function's body is not available here; only the
// surviving control flow is transcribed. param_1+6 matches network_server_globals::flags;
// param_1+0x3c4 matches ::machines[0].machine_id.
// register convention: EAX = passthrough (returned unchanged, masked, whenever the gate at
// server->flags bit0 is clear), ESI = address_out (s_network_address *, forwarded to
// network_channel_get_remote_address), stack = server (network_server_globals *),
// connection (network_receive_queue **, only ever tested for non-NULL and forwarded).
// blam-cc: EAX -> passthrough, ESI -> address_out, stack -> server, connection
// UNSURE: this function's true behaviour is materially larger than what survives Ghidra's
// dead-code elimination (it references network_channel_list_add,
// network_server_build_game_info_packet (FUN_004e0950) and
// network_server_build_full_game_info_packet (FUN_004e0bd0) as callees, none of which appear
// in the reachable code shown here); only the visible fragment is rewritten.
// UNSURE: the early-exit value inside the loop (`(uint)psVar1 & 0xffffff00`, the *pointer*
// one-past the 17th machine slot, masked) looks like leftover register reuse rather than a
// meaningful count; preserved literally.
```

```
#if 0
Original Ghidra decompilation (0x4e0d30):

/* WARNING: Removing unreachable block (ram,0x004e0de3) */
/* WARNING: Removing unreachable block (ram,0x004e0ded) */
/* WARNING: Removing unreachable block (ram,0x004e0df4) */
/* WARNING: Removing unreachable block (ram,0x004e0e00) */
/* WARNING: Removing unreachable block (ram,0x004e0e6a) */
/* WARNING: Removing unreachable block (ram,0x004e0e73) */
/* WARNING: Removing unreachable block (ram,0x004e0e9b) */
/* WARNING: Removing unreachable block (ram,0x004e0ea1) */
/* WARNING: Removing unreachable block (ram,0x004e0eab) */
/* WARNING: Removing unreachable block (ram,0x004e0ec9) */
/* WARNING: Removing unreachable block (ram,0x004e0eb5) */

uint FUN_004e0d30(int param_1,int *param_2)

{
  uint in_EAX;
  short *psVar1;
  int iVar2;

  if ((*(byte *)(param_1 + 6) & 1) != 0) {
    iVar2 = 0;
    psVar1 = (short *)(param_1 + 0x3c4);
    while (*psVar1 != -1) {
      iVar2 = iVar2 + 1;
      psVar1 = psVar1 + 0x30;
      if (0xf < iVar2) {
        return (uint)psVar1 & 0xffffff00;
      }
    }
    if (*param_2 != 0) {
      network_channel_get_remote_address();
    }
    in_EAX = 0;
  }
  return in_EAX & 0xffffff00;
}
#endif
```

## network_server_handle_rcon_request.c

```
// network_server_handle_rcon_request  (Ghidra: already named)
// address 0x4e4f00, size 438 bytes
// name confidence: 0.75   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md ("validates the password, executes the command if
// valid, and reports the result back to the requesting client"); disassembly (objdump -d -M
// intel) confirms EAX -> client record (+0xc machine id) and EDX -> message, and that
// FUN_004ec590 takes the message in EAX and an output buffer in ECX.
// register convention: EAX -> client, EDX -> message.
//   // blam-cc: EAX -> client, EDX -> message
// UNSURE: this whole function reads a message-delta-decoded record whose exact field layout is
// not resolved (types/networking.h documents the message-delta protocol as only partly
// resolved); the password/command split below (a 20-byte password region immediately followed
// by a 64-byte command region) matches the relative order and rough sizes of Ghidra's own
// locals (local_4c/local_4b zeroed as one ~74-byte run, then local_43[64]) but the exact byte
// boundary is not independently confirmed. UNSURE: FUN_004c69a0's signature (foreign, executes
// the decoded command); UNSURE: the "+1" on the machine id in the success-message call, kept
// exactly as Ghidra shows it even though every other branch here logs the plain id.
```

```
#if 0
Original Ghidra decompilation (0x4e4f00), from tools/pack.py 0x4e4f00:

void network_server_handle_rcon_request(void)

{
  byte bVar1;
  char cVar2;
  int in_EAX;
  char *pcVar3;
  byte *pbVar4;
  char *pcVar5;
  int iVar6;
  undefined4 *in_EDX;
  int iVar7;
  byte *pbVar8;
  undefined4 *puVar9;
  bool bVar10;
  byte local_4c;
  undefined4 local_4b;
  undefined1 local_44;
  char local_43 [64];
  undefined1 local_3;

  iVar7 = (int)*(short *)(in_EAX + 0xc);
  if (*(int *)*in_EDX != 0) {
    FUN_004ec670();
    chimera__console_out("Ignoring meaningless rcon_request message from client #%d",iVar7);
    return;
  }
  local_4c = 0;
  puVar9 = &local_4b;
  for (iVar6 = 0x12; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar9 = 0;
    puVar9 = puVar9 + 1;
  }
  *(undefined1 *)puVar9 = 0;
  cVar2 = FUN_004ec590();
  if (cVar2 == '\0') {
    chimera__console_out("Could not decode rcon message from client #%d",iVar7);
    return;
  }
  local_44 = 0;
  local_3 = 0;
  pcVar5 = &DAT_0071c410;
  do {
    pcVar3 = pcVar5;
    pcVar5 = pcVar3 + 1;
  } while (*pcVar3 != '\0');
  if (pcVar3 == &DAT_0071c410) {
    chimera__rcon_out(iVar7);
    chimera__console_out("Ignoring rcon request from client #%d (rcon is disabled)",iVar7);
    return;
  }
  pbVar8 = &local_4c;
  pbVar4 = &DAT_0071c410;
  do {
    bVar1 = *pbVar4;
    bVar10 = bVar1 < *pbVar8;
    if (bVar1 != *pbVar8) {
LAB_004e4fb4:
      iVar6 = (1 - (uint)bVar10) - (uint)(bVar10 != 0);
      goto LAB_004e4fb9;
    }
    if (bVar1 == 0) break;
    bVar1 = pbVar4[1];
    bVar10 = bVar1 < pbVar8[1];
    if (bVar1 != pbVar8[1]) goto LAB_004e4fb4;
    pbVar4 = pbVar4 + 2;
    pbVar8 = pbVar8 + 2;
  } while (bVar1 != 0);
  iVar6 = 0;
LAB_004e4fb9:
  if (iVar6 != 0) {
    chimera__rcon_out(iVar7);
    chimera__console_out("Ignoring rcon request from client #%d (bad password)",iVar7);
    return;
  }
  pcVar5 = local_43;
  do {
    cVar2 = *pcVar5;
    pcVar5 = pcVar5 + 1;
  } while (cVar2 != '\0');
  if (pcVar5 != local_43 + 1) {
    cVar2 = FUN_004c69a0();
    if (cVar2 != '\0') {
      chimera__rcon_out(iVar7);
      chimera__console_out("Successfully executed rcon command from client #%d.",iVar7 + 1);
      return;
    }
    chimera__rcon_out(iVar7);
    chimera__console_out("Failure executing rcon command from client #%d.",iVar7);
    return;
  }
  chimera__rcon_out(iVar7);
  chimera__console_out("Ignoring rcon request from client #%d (empty command)",iVar7);
  return;
}
#endif
```

## network_server_heartbeat_tick.c

```
// network_server_heartbeat_tick  (Ghidra: FUN_004e15a0, unnamed)
// address 0x4e15a0, size 627 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Main per-frame networking heartbeat that
// times out unestablished machines and advances the connection/challenge handshake state
// machine." server+0x9c8/+0x9cc/+0x9d0/+0x9d4/+0x9d5/+0x9d6 are exactly the fields
// network_client_connection_handshake_tick.c reconstructs as a network_timer_pair plus three
// state bytes; server+0x9c4/+0x004/+session.unknown_3ac match the same reset sequence as
// network_game_server_handle_client_join.c; server+0x9bc matches
// network_server_resend_challenge_periodic.c's timestamp exactly (duplicated inline here
// rather than calling that function).
// register convention: EDI = server (network_server_globals *).
// blam-cc: EDI -> server
// UNSURE: FUN_00449210 is called here and its result IS used (unlike its other, argument-less,
// side-effect-only call sites elsewhere in this batch), so it is modelled here as returning an
// int32_t tick value; FUN_004df1c0 and FUN_004e04f0/FUN_004e0480/FUN_004e14e0 are all called
// with no visible arguments in the original even though they need `server`; passed explicitly
// here, consistent with how this batch's other files resolve the same situation.
// UNSURE: bit 0x04 of the ushort read at machine+0xe (flags+unknown_0f combined) in the second
// branch is treated as the same "processed this round" bit used in
// network_game_server_handle_client_join.c, though it is not an enumerated
// network_machine_flags value.
```

```
#if 0
Original Ghidra decompilation (0x4e15a0):

undefined1 FUN_004e15a0(void)

{
  char cVar1;
  undefined1 uVar2;
  int iVar3;
  int iVar4;
  ushort *puVar5;
  int *piVar6;
  int unaff_EDI;
  bool bVar7;
  undefined8 uVar8;
  undefined1 local_11;
  LARGE_INTEGER local_c;

  QueryPerformanceCounter(&local_c);
  uVar8 = __allmul(local_c.s.LowPart,local_c.s.HighPart,1000,0);
  iVar3 = __alldiv(uVar8,DAT_006ac8f8,DAT_006ac8fc);
  local_11 = 1;
  if (*(char *)(unaff_EDI + 0x9f9) == '\0') {
    piVar6 = (int *)(unaff_EDI + 0x3b8);
    iVar4 = 0x10;
    do {
      if ((*piVar6 != 0) && ((~(byte)(*(uint *)(*piVar6 + 0xa8c) >> 4) & 1) == 0)) {
        FUN_004df090(0);
      }
      piVar6 = piVar6 + 0x18;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
    if (*(char *)(unaff_EDI + 0x9d4) == '\x01') {
      cVar1 = FUN_004e04f0();
      if ((cVar1 == '\0') || (cVar1 = FUN_004e0480(), cVar1 != '\0')) {
        bVar7 = false;
        *(undefined4 *)(unaff_EDI + 0x9c8) = 0;
        *(undefined4 *)(unaff_EDI + 0x9cc) = 0;
        *(undefined4 *)(unaff_EDI + 0x9d0) = 0;
        *(undefined4 *)(unaff_EDI + 0x9d4) = 0;
      }
      else {
        bVar7 = true;
        FUN_004deb50();
        if (((*(int *)(unaff_EDI + 0x9c8) == 0) && (cVar1 = FUN_004e14e0(), cVar1 != '\0')) &&
           (*(char *)(unaff_EDI + 0x9d5) == '\0')) {
          local_11 = FUN_004df1c0();
          goto LAB_004e17e9;
        }
        if (iVar3 - *(int *)(unaff_EDI + 0x9d0) < 0x3e9) goto LAB_004e17e9;
      }
      *(undefined1 *)(unaff_EDI + 0x9d6) = 0;
      if (bVar7) {
        FUN_004deb50();
      }
      iVar4 = network_prepare_challenge_packet();
      if ((iVar4 != 0) && (cVar1 = FUN_004e19c0(0,iVar4,1,0,1,3), cVar1 != '\0')) {
        *(int *)(unaff_EDI + 0x9d0) = iVar3;
      }
    }
    else if (*(int *)(unaff_EDI + 0x9bc) + 5000 < iVar3) {
      iVar4 = network_prepare_challenge_packet();
      FUN_004e19c0(0,iVar4,1,0,1,3);
      *(int *)(unaff_EDI + 0x9bc) = iVar3;
    }
  }
  else if ((*(int *)(unaff_EDI + 0x9c4) != 0) &&
          (iVar3 = FUN_00449210(), 59999 < (uint)(iVar3 - *(int *)(unaff_EDI + 0x9c4)))) {
    puVar5 = (ushort *)(unaff_EDI + 0x3c6);
    iVar3 = 0x10;
    do {
      if (((*puVar5 & 1) != 0) && ((*puVar5 & 4) == 0)) {
        FUN_004df090(0);
      }
      iVar4 = DAT_0071c2d8;
      puVar5 = puVar5 + 0x30;
      iVar3 = iVar3 + -1;
    } while (iVar3 != 0);
    bVar7 = DAT_0071c2d8 == 0;
    *(undefined2 *)(unaff_EDI + 4) = 1;
    *(undefined4 *)(unaff_EDI + 0x9c4) = 0;
    if (bVar7) {
      uVar2 = 0;
    }
    else {
      uVar2 = *(undefined1 *)(iVar4 + 0xec0);
    }
    *(undefined1 *)(unaff_EDI + 0x3b4) = uVar2;
  }
LAB_004e17e9:
  if (*(char *)(unaff_EDI + 0x9fa) == '\x01') {
    cVar1 = FUN_004e0720();
    if (cVar1 == '\x01') {
      *(undefined2 *)(unaff_EDI + 4) = 1;
    }
    *(undefined1 *)(unaff_EDI + 0x9fa) = 0;
  }
  return local_11;
}
#endif
```

## network_server_notify_or_resend_challenge.c

```
// network_server_notify_or_resend_challenge  (Ghidra: FUN_004e0af0)
// address 0x4e0af0, size 150 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x4e0af0..0x4e0b85: CX reason, EDI machine, stack server. For a machine whose
//   channel is connected (+0xa98): the chat close deadline (0x00718fa4, when unset) becomes reason + 0x2b, the host
//   hand-off flag is set, chat closes; returns 1. Otherwise a type 6 packet carrying the reason goes to the machine
//   (reliable, 3) and the machine timer restarts for 1000 ms; returns whether the send worked (0 when no packet was
//   built). The server argument was missing.
// blam-cc: CX -> reason, EDI -> machine, stack -> server
```

```
#if 0
Original Ghidra decompilation (0x4e0af0):

undefined1 FUN_004e0af0(void)

{
  char cVar1;
  ushort *puVar2;
  short in_CX;
  undefined1 uVar3;
  int *unaff_EDI;

  uVar3 = 1;
  if (((unaff_EDI != (int *)0x0) && (*unaff_EDI != 0)) && (*(char *)(*unaff_EDI + 0xa98) != '\0')) {
    if (DAT_00718fa4 == -1) {
      DAT_00718fa4 = in_CX + 0x2b;
    }
    DAT_0071c2de = 1;
    chat_close();
    return 1;
  }
  puVar2 = (ushort *)network_prepare_challenge_packet();
  if (puVar2 != (ushort *)0x0) {
    cVar1 = network_session_send_to_machine(0,puVar2,(uint)(*puVar2 >> 4) << 3,1,1,0,3);
    if (cVar1 != '\0') goto LAB_004e0b4d;
  }
  uVar3 = 0;
LAB_004e0b4d:
  FUN_004df090(1000);
  return uVar3;
}
#endif
```

## network_server_password_get.c

```
// network_server_password_get  (Ghidra: FUN_004e0930, unnamed)
// address 0x4e0930, size 24 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Copies the session's stored password
// (offset 0x9fc) out into the caller-supplied wide-character buffer."
// register convention: EAX = server (network_server_globals *), ESI = dest (wchar_t *).
// blam-cc: EAX -> server, ESI -> dest
```

```
#if 0
Original Ghidra decompilation (0x4e0930):

void FUN_004e0930(void)

{
  int in_EAX;
  wchar_t *unaff_ESI;

  _wcsncpy(unaff_ESI,(wchar_t *)(in_EAX + 0x9fc),8);
  unaff_ESI[8] = L'\0';
  return;
}
#endif
```

## network_server_password_is_set.c

```
// network_server_password_is_set  (Ghidra: FUN_004e08e0, unnamed)
// address 0x4e08e0, size 34 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Returns true if the session's wide-character
// password field at offset 0x9fc is non-empty." Matches types/networking.h's
// network_server_globals::password exactly.
// register convention: EAX = server (network_server_globals *).
// blam-cc: EAX -> server
```

```
#if 0
Original Ghidra decompilation (0x4e08e0):

bool FUN_004e08e0(void)

{
  int in_EAX;
  int iVar1;

  iVar1 = _wcsncmp((wchar_t *)(in_EAX + 0x9fc),L"",8);
  return iVar1 != 0;
}
#endif
```

## network_server_password_set.c

```
// network_server_password_set  (Ghidra: FUN_004e0910, unnamed)
// address 0x4e0910, size 28 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Copies up to 8 wide characters into the
// connection/session object's password field at offset 0x9fc." Matches
// network_server_globals::password (9 uint16_t, forced NUL at 0xa0c).
// register convention: EAX = source (const wchar_t *), ESI = server (network_server_globals *).
// blam-cc: EAX -> source, ESI -> server
```

```
#if 0
Original Ghidra decompilation (0x4e0910):

void FUN_004e0910(void)

{
  wchar_t *in_EAX;
  int unaff_ESI;

  _wcsncpy((wchar_t *)(unaff_ESI + 0x9fc),in_EAX,8);
  *(undefined2 *)(unaff_ESI + 0xa0c) = 0;
  return;
}
#endif
```

## network_server_resend_challenge_periodic.c

```
// network_server_resend_challenge_periodic  (Ghidra: FUN_004e1450, unnamed)
// address 0x4e1450, size 142 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Resends a prepared challenge/info packet to a
// peer roughly every 5 seconds while a connection attempt is outstanding." server+0x9bc falls
// inside network_server_globals::unknown_9bc (no individual field name available).
// register convention: ESI = server (network_server_globals *).
// blam-cc: ESI -> server
```

```
#if 0
Original Ghidra decompilation (0x4e1450):

undefined4 FUN_004e1450(void)

{
  uint uVar1;
  int iVar2;
  int unaff_ESI;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar1 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
  if (*(int *)(unaff_ESI + 0x9bc) + 5000U < uVar1) {
    iVar2 = network_prepare_challenge_packet();
    FUN_004e19c0(0,iVar2,1,0,1,3);
    *(uint *)(unaff_ESI + 0x9bc) = uVar1;
  }
  return 1;
}
#endif
```

## network_server_service_machines_tick.c

```
// network_server_service_machines_tick  (Ghidra: FUN_004e11d0, unnamed)
// address 0x4e11d0, size 191 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Per-frame pass over the session's 16 machine
// slots that advances pending connections and checks established ones for timeout." The
// channel-flags tests (`~(flags>>4)&1` = not k_network_channel_dead, `flags&6` =
// k_network_channel_client|k_network_channel_transmit_pending) and the endpoint's flags bit0
// (connection-oriented) all match types/networking.h exactly. FUN_004df090 sets exactly the
// timer_14/timer_18 fields that network_server_check_machine_timeout.c reads, confirming it
// operates on a network_machine (or an object sharing that layout).
// register convention: the single parameter is a genuine stack (cdecl) parameter.
// blam-cc: stack -> server
// UNSURE: FUN_004df090's real name/signature; called here with a literal 0 duration.
```

```
#if 0
Original Ghidra decompilation (0x4e11d0):

char FUN_004e11d0(int param_1)

{
  char cVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  int *piVar5;

  iVar4 = 0;
  cVar2 = '\x01';
  piVar5 = (int *)(param_1 + 0x3b8);
  do {
    if (0xf < iVar4) {
      return cVar2;
    }
    if ((short)piVar5[3] != -1) {
      if ((char)piVar5[4] == '\0') {
        if (((((~(byte)(*(uint *)(*piVar5 + 0xa8c) >> 4) & 1) == 0) ||
             (cVar1 = FUN_004dd110(0), cVar1 == '\0')) ||
            ((*(byte *)((int *)*piVar5 + 0x2a3) & 6) == 0)) ||
           ((iVar3 = *(int *)*piVar5, iVar3 == 0 || ((*(byte *)(iVar3 + 0xc) & 1) == 0)))) {
          FUN_004df090(0);
        }
        else {
          cVar2 = FUN_004e1290(param_1,piVar5);
          if (cVar2 == '\0') {
            FUN_004df090(0);
            cVar2 = '\x01';
          }
        }
        if ((char)piVar5[4] == '\0') goto LAB_004e127c;
      }
      iVar3 = FUN_004e0ef0(param_1,piVar5);
      if (iVar3 == 0) {
        cVar2 = '\0';
      }
    }
LAB_004e127c:
    iVar4 = iVar4 + 1;
    piVar5 = piVar5 + 0x18;
    if (cVar2 == '\0') {
      return '\0';
    }
  } while( true );
}
#endif
```

## network_server_status_periodic_print.c

```
// network_server_status_periodic_print  (Ghidra: FUN_004e1520, unnamed)
// address 0x4e1520, size 120 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Periodically (every ~15s) prints the
// dedicated server status by invoking sv_status when the relevant channel flag is active."
// param_1+6 matches network_server_globals::flags (stats-logging bit).
// register convention: stack = server (network_server_globals *).
// blam-cc: stack -> server
// UNSURE: DAT_0071c2f0 (the "last printed" timestamp) has no established name elsewhere.
```

```
#if 0
Original Ghidra decompilation (0x4e1520):

undefined4 FUN_004e1520(int param_1)

{
  int iVar1;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  if ((*(byte *)(param_1 + 6) >> 2 & 1) != 0) {
    QueryPerformanceCounter(&local_8);
    uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    iVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
    if (15000 < (uint)(iVar1 - DAT_0071c2f0)) {
      sv_status();
      DAT_0071c2f0 = iVar1;
    }
  }
  return 1;
}
#endif
```

## network_server_validate_join_request.c

```
// network_server_validate_join_request  (Ghidra: FUN_004e0850, unnamed)
// address 0x4e0850, size 141 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Validates a connecting client's build/version
// and channel state against accepted ranges and returns a numeric status/error code, checking
// for a free machine slot on success." `&DAT_0087bc1c + count*0x14` lands on
// network_pending_connections[count-1].first_payload_word (network_pending_connection is 0x14
// bytes, first_payload_word at +0x10, base 0x0087bc20 = 0x0087bc1c + 0x14 * 1); +0x1a8/+0x1a5
// (relative to server, session at +0x008) are session.player_count (session+0x1a0) and
// session.maximum_players (session+0x19d); +6 and +0x3c4 are ::flags and
// ::machines[0].machine_id, matching every sibling function in this batch.
// register convention: EDX = server (network_server_globals *).
// VERIFIED against disassembly 0x4e0850..0x4e08dc (2026-09-30): the pending entry word (+0x10 of the last entry), the
//   0x96640 range split, the room test and the machine scan match; fixed: maximum_players is sign-extended (movsx).
//   The build word is read from an uninitialised stack slot in the original when the queue is empty, which is why a
//   difftest on an empty queue can return 4 vs 5 without either being wrong.
// blam-cc: EDX -> server
// UNSURE: `local_4` is left uninitialized in the original when
// network_pending_connection_count is not positive (a genuine read of an indeterminate stack
// value); preserved as an uninitialized local rather than defaulting it to 0.
// UNSURE: the numeric return codes (0, 4, 5, 6, 7) have no established meaning beyond what
// this function's own branches imply (0 = ok, 5 = build too new, 6 = no room, 7 = session not
// ready, 4 = build too old).
```

```
#if 0
Original Ghidra decompilation (0x4e0850):

undefined4 FUN_004e0850(void)

{
  int iVar1;
  int in_EDX;
  short *psVar2;
  int local_4;

  if (0 < DAT_006f16d0) {
    local_4 = *(int *)(&DAT_0087bc1c + DAT_006f16d0 * 0x14);
  }
  if (0x9663f < local_4) {
    if (0x96640 < local_4) {
      return 5;
    }
    if (*(short *)(in_EDX + 0x1a8) < (short)*(char *)(in_EDX + 0x1a5)) {
      if ((*(byte *)(in_EDX + 6) & 1) == 0) {
        return 7;
      }
      iVar1 = 0;
      psVar2 = (short *)(in_EDX + 0x3c4);
      do {
        if (*psVar2 == -1) {
          return 0;
        }
        iVar1 = iVar1 + 1;
        psVar2 = psVar2 + 0x30;
      } while (iVar1 < 0x10);
    }
    return 6;
  }
  return 4;
}
#endif
```

## network_session_broadcast_to_all.c

```
// network_session_broadcast_to_all  (Ghidra: FUN_004e19c0, unnamed)
// address 0x4e19c0, size 183 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Broadcasts a prepared packet to every
// established machine in the session's machine table." Channel dead-bit and ::connected tests
// match every other send path in this batch exactly.
// register convention: ECX = server (network_server_globals *), stack = param_1, data,
// param_3, param_4, force (char), param_6.
// blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6
// UNSURE: the per-machine gate tests flags bit 0x02 (k_network_machine_pending per
// types/networking.h), not bit 0x01 (k_network_machine_established) as the "every established
// machine" summary would suggest; preserved literally.
```

```
#if 0
Original Ghidra decompilation (0x4e19c0):

undefined1
FUN_004e19c0(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,char param_5,
            undefined4 param_6)

{
  char cVar1;
  int iVar2;
  int in_ECX;
  int *piVar3;
  undefined1 local_6;
  undefined1 local_5;
  int local_4;

  local_6 = 1;
  piVar3 = (int *)(in_ECX + 0x3b8);
  local_4 = 0x10;
  do {
    if (piVar3 == (int *)0x0) {
      iVar2 = 0;
    }
    else {
      iVar2 = *piVar3;
    }
    cVar1 = '\0';
    if (iVar2 != 0) {
      cVar1 = *(char *)(iVar2 + 0xa98);
    }
    if ((((*(byte *)((int)piVar3 + 0xe) >> 1 & 1) != 0) &&
        (((cVar1 != '\x01' || (param_5 != '\0')) && (*piVar3 != 0)))) &&
       ((~(byte)(*(uint *)(*piVar3 + 0xa8c) >> 4) & 1) != 0)) {
      local_5 = param_1 != 0;
      cVar1 = FUN_004dce40(param_2,&local_5,1,param_3,param_4,param_6);
      if (cVar1 == '\0') {
        local_6 = 0;
      }
    }
    piVar3 = piVar3 + 0x18;
    local_4 = local_4 + -1;
  } while (local_4 != 0);
  return local_6;
}
#endif
```

## network_session_broadcast_to_flagged.c

```
// network_session_broadcast_to_flagged  (Ghidra: FUN_004e1a80, unnamed)
// address 0x4e1a80, size 193 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Broadcasts a prepared packet only to machines
// whose table entry has an extra qualifying flag bit set, on top of being established."
// Identical structure to network_session_broadcast_to_all.c, with an added flags-bit-0x04
// test.
// register convention: ECX = server (network_server_globals *), stack = status_bit, data,
// immediate, flush_after, force (char), unused.
// blam-cc: EAX -> body_bit_count, ECX -> server, stack -> status_bit, data, immediate, flush_after, force, unused (see FIXED below)
```

```
#if 0
Original Ghidra decompilation (0x4e1a80):

undefined1
FUN_004e1a80(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,char param_5,
            undefined4 param_6)

{
  byte bVar1;
  char cVar2;
  int iVar3;
  int in_ECX;
  int *piVar4;
  undefined1 local_6;
  undefined1 local_5;
  int local_4;

  local_6 = 1;
  piVar4 = (int *)(in_ECX + 0x3b8);
  local_4 = 0x10;
  do {
    if (piVar4 == (int *)0x0) {
      iVar3 = 0;
    }
    else {
      iVar3 = *piVar4;
    }
    cVar2 = '\0';
    if (iVar3 != 0) {
      cVar2 = *(char *)(iVar3 + 0xa98);
    }
    bVar1 = (byte)*(undefined2 *)((int)piVar4 + 0xe);
    if (((((bVar1 >> 1 & 1) != 0) && ((bVar1 >> 2 & 1) != 0)) &&
        ((cVar2 != '\x01' || (param_5 != '\0')))) &&
       ((*piVar4 != 0 && ((~(byte)(*(uint *)(*piVar4 + 0xa8c) >> 4) & 1) != 0)))) {
      local_5 = param_1 != 0;
      cVar2 = FUN_004dce40(param_2,&local_5,1,param_3,param_4,param_6);
      if (cVar2 == '\0') {
        local_6 = 0;
      }
    }
    piVar4 = piVar4 + 0x18;
    local_4 = local_4 + -1;
  } while (local_4 != 0);
  return local_6;
}
#endif
```

## network_session_send_to_machine.c

```
// network_session_send_to_machine  (Ghidra: network_session_send_to_machine, already named)
// address 0x4e1930, size 137 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Sends a prepared packet to the single machine
// identified by id, skipping machines that are in the process of disconnecting." status_bit+0x3c4
// matches network_server_globals::machines[0].machine_id; the found machine's channel is
// checked against ::connected (+0xa98) exactly as in every other send path in this batch.
// register convention: EAX = machine_id (int32_t), ESI = server (network_server_globals *),
// stack = status_bit (unused), data, body_bit_count (unused), reliable, unknown_a, force (char),
// priority.
// blam-cc: EAX -> machine_id, ESI -> server, stack -> (unused, data, unused, reliable,
// unknown_a, force, priority)
// UNSURE: status_bit and body_bit_count are genuinely unused stack parameters in the original (only
// param_2, param_4, param_5, param_7 are forwarded to network_channel_queue_message, plus a literal 1 in
// body_bit_count's slot). This batch's other files that call this function pass their arguments
// positionally as if `machine_id` were an ordinary leading stack argument (matching how every
// call site's constant literal doubles as both the discarded status_bit and, presumably, EAX);
// that mismatch with this function's own true ABI is a known inconsistency in this batch, not
// resolved here.
// UNSURE: network_channel_queue_message's real parameter meaning is inferred purely from this call site.
```

```
#if 0
Original Ghidra decompilation (0x4e1930):

uint network_session_send_to_machine
               (undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
               undefined4 param_5,char param_6,undefined4 param_7)

{
  int *piVar1;
  uint in_EAX;
  uint uVar2;
  int iVar3;
  short *psVar4;
  int unaff_ESI;
  undefined1 local_1;

  uVar2 = in_EAX & 0xffffff00;
  iVar3 = 0;
  psVar4 = (short *)(unaff_ESI + 0x3c4);
  do {
    if ((int)*psVar4 == in_EAX) {
      piVar1 = (int *)(iVar3 * 0x60 + 0x3b8 + unaff_ESI);
      if ((((piVar1 != (int *)0x0) && (iVar3 = *piVar1, iVar3 != 0)) &&
          ((*(char *)(iVar3 + 0xa98) != '\x01' || (param_6 != '\0')))) && (iVar3 != 0)) {
        uVar2 = FUN_004dce40(param_2,&local_1,1,param_4,param_5,param_7);
      }
      return uVar2;
    }
    iVar3 = iVar3 + 1;
    psVar4 = psVar4 + 0x30;
  } while (iVar3 < 0x10);
  return uVar2;
}
#endif
```
