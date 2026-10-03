# Original notes: networking `net1_client`

Author notes and decompile blocks moved verbatim from the C files converted into the net1_client sources.

## network_client_begin_connect.c

```
// network_client_begin_connect  (Ghidra: FUN_004dc8d0; named per this rewrite)
// address 0x4dc8d0, size 210 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Begins a new outgoing connection attempt:
// allocates/reuses the client connection object, hands off to chimera__on_connect with the
// requested name/options, and marks the network session active on success." Confirmed via
// objdump against network_game_client_connect_to_address.c's call site (`lea ecx,[esp+0xc]`
// right before `call 0x4dc8d0`) that the elided ECX argument is the caller's local
// s_network_address; this function's own body reads it as `in_ECX->ipv4 != 0` and
// `in_ECX->port != 0` before proceeding, which is exactly chimera__on_connect's own
// `target_address` parameter passed straight through.
// register/parameter convention: blam-cc: ECX -> target_address, stack -> player_name
// UNSURE: `*(uint *)(client + 0xf4c) = (uint)local_1048` writes one byte past the end of
// network_client_globals (documented size 0x0f4c); local_1048 is itself never assigned in the
// decompiled body (it sits immediately after the oversized DAT_00712dd8 copy destination, and
// reads as compiler/stack-frame scaffolding around the large local frame's __chkstk probe, not
// hand-written Blam data). Preserved as a raw out-of-struct write of 0 (the closest stand-in for
// an otherwise-uninitialized value) rather than silently dropped, per the no-invented-behaviour
// rule, but this is the single largest source of doubt in this file's rewrite confidence.
// UNSURE: the local scratch this function builds for chimera__on_connect's `session_info`
// argument is, byte for byte, [2 unused bytes][8-wide-char player name][forced NUL][the leading
// bytes of a 0x7ff-dword copy from profile_globals_block] -- same "oversized copy
// into an undersized local, kept verbatim" situation server_browser_open.c already documents for
// the same global.
// UNSURE: network_debug_fill_canary_buffer (0x4e0790, outside this batch's range) is called with no visible
// arguments at its only call site; left as a bare call.
```

```
#if 0
Original Ghidra decompilation (0x4dc8d0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_004dc8d0(wchar_t *param_1)

{
  short *psVar1;
  char cVar2;
  uint uVar3;
  int *in_ECX;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined1 local_202c [2];
  wchar_t local_202a [8];
  undefined2 local_201a;
  undefined4 local_2008 [1008];
  byte local_1048;
  undefined4 uStack_c;

  uVar3 = 0x202c;
  uStack_c = 0x4dc8e0;
  if (DAT_0071c2d8 == 0) {
    uVar3 = network_session_create();
    DAT_0071c2d8 = uVar3;
    if (uVar3 != 0) {
      DAT_0071c2de = 0;
    }
  }
  puVar5 = &DAT_00712dd8;
  puVar6 = local_2008;
  for (iVar4 = 0x7ff; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar6 = puVar6 + 1;
  }
  uVar3 = uVar3 & 0xffffff00;
  psVar1 = (short *)(DAT_0071c2d8 + 0xeda);
  *(uint *)(DAT_0071c2d8 + 0xf4c) = (uint)local_1048;
  if (((*psVar1 == 0) && (*in_ECX != 0)) && (*(short *)((int)in_ECX + 0x12) != 0)) {
    _wcsncpy(local_202a,param_1,8);
    local_201a = 0;
    FUN_004e0790();
    cVar2 = chimera__on_connect(DAT_0071c2d8,local_202c);
    if (cVar2 != '\0') {
      DAT_00719720 = 1;
      return 1;
    }
    DAT_0071c2de = 1;
    uVar3 = chat_close();
    uVar3 = uVar3 & 0xffffff00;
  }
  return uVar3;
}
#endif
```

## network_client_check_connection_quality.c

```
// network_client_check_connection_quality  (Ghidra: FUN_004e0080, unnamed)
// address 0x4e0080, size 504 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Tracks a client's packet loss/latency using
// the high-resolution timer, and returns false to reject/kick the client once its measured
// loss or latency exceeds the hard-coded thresholds for too many cons[ecutive samples]."
// machine_to_player/datum_get idiom matches the sibling lookups in this batch; player+0x108,
// +0x10c, +0x110, +0x114, +0x118 match types/game.h's player::unknown_108/10c/110/114/118
// exactly (five previously-unnamed fields, now confirmed to be a per-player loss/latency
// sample block).
// register convention: EAX = machine_index (uint32_t), stack = units (uint8_t, a
// packets/bytes-since-last-call count).
// blam-cc: EAX -> machine_index, stack -> units
// UNSURE: DAT_006894a8 (a byte gate) and the DAT_00719700/DAT_00719704 pair (a cached
// QueryPerformanceCounter-style LARGE_INTEGER, multiplied by 1000 and divided by
// performance_frequency to get milliseconds) have no established names elsewhere in this
// module; declared locally with generic names.
// UNSURE: the float thresholds (36.0 loss ratio, 5000ms window, 39.9ms latency, 5 consecutive
// bad samples) are transcribed as literals exactly as decompiled.
```

```
#if 0
Original Ghidra decompilation (0x4e0080):

undefined4 FUN_004e0080(byte param_1)

{
  float fVar1;
  float fVar2;
  short sVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  uint uVar6;
  short sVar7;
  uint uVar8;
  int iVar9;
  undefined8 uVar10;
  float local_8;

  iVar4 = (&DAT_006b1460)[in_EAX & 0xffff];
  if (((iVar4 != -1) && (sVar3 = (short)iVar4, -1 < sVar3)) &&
     (sVar3 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar9 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar3;
    sVar3 = *(short *)(iVar9 + *(int *)(DAT_0087a480 + 0x34));
    iVar9 = iVar9 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar3 != 0) && ((sVar7 = (short)((uint)iVar4 >> 0x10), sVar7 == 0 || (sVar3 == sVar7)))) {
      if (DAT_006894a8 == '\x01') {
        uVar10 = __allmul(DAT_00719700,DAT_00719704,1000,0);
        iVar4 = __alldiv(uVar10,DAT_006ac8f8,DAT_006ac8fc);
        uVar8 = (uint)param_1;
        if (*(char *)(iVar9 + 0x108) == '\0') {
          *(int *)(iVar9 + 0x10c) = iVar4;
          *(uint *)(iVar9 + 0x110) = uVar8;
          *(int *)(iVar9 + 0x114) = iVar4;
          *(undefined4 *)(iVar9 + 0x118) = 0;
          *(undefined1 *)(iVar9 + 0x108) = 1;
        }
        else {
          iVar5 = *(int *)(iVar9 + 0x110) + uVar8;
          *(int *)(iVar9 + 0x110) = iVar5;
          uVar6 = iVar4 - *(int *)(iVar9 + 0x10c);
          if (uVar6 == 0) {
            fVar1 = 0.0;
          }
          else {
            fVar1 = (float)iVar5;
            if (iVar5 < 0) {
              fVar1 = fVar1 + 4.2949673e+09;
            }
            fVar2 = (float)(int)uVar6;
            if ((int)uVar6 < 0) {
              fVar2 = fVar2 + 4.2949673e+09;
            }
            fVar1 = fVar1 / (fVar2 * 0.001);
          }
          if (10000 < uVar6) {
            *(int *)(iVar9 + 0x10c) = iVar4;
            *(uint *)(iVar9 + 0x110) = uVar8;
            uVar6 = 0;
          }
          iVar5 = iVar4 - *(int *)(iVar9 + 0x114);
          if (iVar5 == 0) {
            local_8 = 0.0;
          }
          else {
            fVar2 = (float)iVar5;
            if (iVar5 < 0) {
              fVar2 = fVar2 + 4.2949673e+09;
            }
            local_8 = (float)uVar8 / (fVar2 * 0.001);
          }
          *(int *)(iVar9 + 0x114) = iVar4;
          if ((36.0 < fVar1) && (5000 < uVar6)) {
            return 0;
          }
          if (local_8 != 0.0) {
            if (local_8 <= 39.9) {
              *(undefined4 *)(iVar9 + 0x118) = 0;
              return 1;
            }
            iVar4 = *(int *)(iVar9 + 0x118) + 1;
            *(int *)(iVar9 + 0x118) = iVar4;
            if (5 < iVar4) {
              return 0;
            }
          }
        }
      }
      return 1;
    }
  }
  return 0;
}
#endif
```

## network_client_connect_progress_percent.c

```
// network_client_connect_progress_percent  (Ghidra: FUN_004d8c10; renamed, no prior name)
// address 0x4d8c10, size 52 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Computes the connection-attempt
// progress as a percentage of the configured connect timeout, for display while in the
// 'connecting' state"); the sole caller (network_client_update_dispatch) assigns its result straight through,
// consistent with the mode check against client+0xeda == 1 (the "waiting to join" state per
// src/networking/network_client_state_dispatch.c case 1).
// register convention: the client pointer arrives in ESI (unaff_ESI), the output percentage
// slot in EDI (unaff_EDI). // blam-cc: ESI -> client, EDI -> out_percent
// UNSURE: the function's *return value* is client->state (the connection-mode field, see
// network_client_state_dispatch.c's note on that field's real meaning), not the percentage --
// the percentage is written only through *out_percent when the mode is 1. Preserved exactly;
// this looks intentional (callers care about the mode transition, and separately snapshot the
// percentage through the pointer for UI text) rather than a decompiler artifact.
// UNSURE: client+0xae4 (byte offset 4 into the still-unresolved network_client_globals
// unknown_ae0[0x34] block) is read here as an int32 millisecond timestamp -- almost certainly
// "connect attempt started at", based on the elapsed-time-over-timeout arithmetic -- but is left
// as a raw offset into that block rather than a new named field, since types/networking.h has
// not resolved it and this task must not redefine that header.
// UNSURE: network_connect_timeout_ms (0x006894ac) is not yet declared in types/networking.h;
// named here from its role as the divisor of an elapsed-ms-times-100 percentage, also read by
// network_channel_attempt_connect and network_join_connect_retry_tick in this module.
```

```
#if 0
Original Ghidra decompilation (0x4d8c10):

undefined2 FUN_004d8c10(void)

{
  int iVar1;
  int unaff_ESI;
  undefined2 *unaff_EDI;

  if ((unaff_EDI != (undefined2 *)0x0) && (*unaff_EDI = 0, *(short *)(unaff_ESI + 0xeda) == 1)) {
    iVar1 = FUN_00449210();
    *unaff_EDI = (short)((uint)((iVar1 - *(int *)(unaff_ESI + 0xae4)) * 100) / DAT_006894ac);
  }
  return *(undefined2 *)(unaff_ESI + 0xeda);
}
#endif
```

## network_client_connection_handshake_tick.c

```
// network_client_connection_handshake_tick  (Ghidra: FUN_004e0590, unnamed)
// address 0x4e0590, size 383 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "State machine that drives the client
// connection handshake/timeout: while connected, updates the disconnect-timeout timer per
// incoming message id, and while not yet connected, (re)starts the timeout ti[mer]." The four
// FUN_004debXX callees are types/networking.h's network_timer_pair helpers, already rewritten
// in this repo (network_timer_advance/_start/_increment_clamped/_decrement_floored).
// register convention: EAX = state (int16_t, a message/step id 0..3), ECX = owner (the object
// whose unknown_9bc-range bytes this function reads/writes).
// blam-cc: EAX -> state, ECX -> owner
// UNSURE (structural): every network_timer_pair call in the original (FUN_004debb0/_50/_d0/_f0)
// is made with no visible arguments even though their real signatures need a timer pointer
// (and, for two of them, extra int32 arguments). The only way the offsets this function itself
// touches (owner+0x9c8, +0x9d0, +0x9d4, +0x9d5, +0x9d6) stay self-consistent is if the timer
// object is `(network_timer_pair *)(owner + 0x9c8)` -- i.e. a compile-time-constant offset
// from `owner`, not a separately passed register -- so that is what is reconstructed here.
// UNSURE: `owner`'s real type is not confirmed; the touched offsets fall exactly inside
// network_server_globals::unknown_9bc (0x9bc..0x9f7), so it is typed that way here, even
// though the function's own summary describes client-side behaviour.
// UNSURE: FUN_00449210 and FUN_004ddd20 (distinct from the global DAT_0071c2dc) have no
// established signatures; called argument-less, matching Ghidra.
// UNSURE: the increment/decrement magnitudes passed to network_timer_increment_clamped and
// network_timer_decrement_floored at the two argument-less call sites are not recoverable
// here; passed as 0 with this note, matching this codebase's precedent for eliding arguments
// it cannot reconstruct (see network_channel_new.c).
```

```
#if 0
Original Ghidra decompilation (0x4e0590):

void FUN_004e0590(void)

{
  char cVar1;
  short in_AX;
  short sVar2;
  int in_ECX;

  if ((*(char *)(in_ECX + 0x9d5) == '\0') &&
     (((cVar1 = FUN_004e04f0(), cVar1 != '\0' && (cVar1 = FUN_004e0480(), cVar1 == '\0')) ||
      (in_AX == 2)))) {
    if (*(char *)(in_ECX + 0x9d4) == '\x01') {
      if (*(char *)(in_ECX + 0x9d6) == '\0') {
        switch(in_AX) {
        case 0:
          *(undefined1 *)(in_ECX + 0x9d6) = 1;
          FUN_004debb0();
          return;
        case 1:
          *(undefined1 *)(in_ECX + 0x9d6) = 1;
          FUN_004deb50();
          if (999 < *(int *)(in_ECX + 0x9c8)) {
            FUN_004debd0();
            FUN_004deb50();
            if (*(int *)(in_ECX + 0x9c8) < 999) {
              FUN_004debf0(999);
              return;
            }
          }
          break;
        case 2:
          *(undefined1 *)(in_ECX + 0x9d6) = 1;
          *(undefined1 *)(in_ECX + 0x9d4) = 0;
          return;
        case 3:
          *(undefined1 *)(in_ECX + 0x9d6) = 1;
          FUN_004debf0(0);
          return;
        }
      }
    }
    else {
      FUN_00449210();
      if (in_AX == 3) {
        FUN_004debf0(0);
        *(undefined1 *)(in_ECX + 0x9d4) = 1;
        *(undefined1 *)(in_ECX + 0x9d6) = 0;
        return;
      }
      if ((DAT_0071c2dc == '\0') || (sVar2 = FUN_004e1880(), 0 < sVar2)) {
        cVar1 = FUN_004ddd20();
        *(undefined1 *)(in_ECX + 0x9d4) = 1;
        FUN_004debf0((-(uint)(cVar1 != '\0') & 0xffffb1e0) + 30999);
        *(undefined4 *)(in_ECX + 0x9d0) = 0;
        *(undefined1 *)(in_ECX + 0x9d6) = 0;
      }
    }
  }
  return;
}
#endif
```

## network_client_drain_queued_updates.c

```
// network_client_drain_queued_updates  (Ghidra: FUN_004e1f40; named per this rewrite, matching
// the name src/networking/network_channel_dispatch_bitstream_unit.c's own extern already chose
// for this address)
// address 0x4e1f40, size 288 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (REWRITTEN; was 0.25)
// evidence: out/phase4/networking_functions.md: "Drains the client's queued network-game
// update messages, applying each one according to its message-type tag."
// register convention: EBX = machine (network_machine *, implicit passthrough -- needed only
// for the type-0xd case, which forwards it to network_game_client_apply_received_update per
// that file's own EBX -> machine convention), stack = param_1 (network_server_globals *,
// forwarded unchanged to the type-0x34 handler, which reads it as `server`), stack = param_2
// (opaque, forwarded unchanged to two foreign handlers).
//   // blam-cc: EBX -> machine, stack -> param_1, param_2
// TYPES-GAP CLOSED 2026-09-20 (review pass): the queued-message record message_delta_decode_array_field fills is
// types/networking.h message_delta_decode_state. The local typedef this file used to carry has
// been folded into the header, where it now also covers decode_context slot 0 in the
// 0x4e5390..0x4e6510 client update family. The join is message_delta_read_changed_subfields
// (0x4ed1d0), which takes the same record in EDI and reads message_type at +0x04 to index
// message_delta_definitions and the bit cursor at +0x10 -- the same +0x04 this function switches
// on. Ghidra split it into local_bc[28] + local_a0 + local_9f only because it could not prove
// they are one object.
// UNSURE (major): message_delta_decode_begin and message_delta_decode_array_field (both foreign, message-delta protocol) are
// called with zero visible arguments; FUN_004aabd0, FUN_00470810 and
// network_server_handle_rcon_request are likewise called with fewer arguments than a real
// implementation would need. All are declared and called exactly as Ghidra shows.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4e1f40..0x4e2060, jump tables 0x4e2078 /
// 0x4e2060): the host twin of network_game_action_queue_drain. Arguments: the item's stream in ECX, then (server,
// machine) on the stack. A 0x34-byte decode state is begun on the stream (EAX state, EDI stream) and the decode
// context built (context[0] the state, [1..16] zero, [0x11] a 0x80-byte record); each decoded item of type 0xd
// (network_game_client_apply_received_update: EBX machine, stack server, context), 0xf (chat_server_relay_incoming_
// message: EAX context, stack machine), 0x1a (game_engine_update_lead_change_state: EAX context, stack machine),
// 0x34 (network_game_message_handle_ping_timestamp: EAX context, stack server) or 0x36
// (network_server_handle_rcon_request: EAX machine, EDX context) is applied; the run continues while both state
// flags +0x1c/+0x1d came back 1 and the count (+0x18) has not passed +0x08. Returns the last result.
```

```
#if 0
Original Ghidra decompilation (0x4e1f40), from tools/pack.py 0x4e1f40:

void FUN_004e1f40(undefined4 param_1,undefined4 param_2)

{
  bool bVar1;
  undefined1 *puVar2;
  char cVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined1 *local_108;
  undefined4 local_104 [16];
  undefined1 *local_c4;
  undefined1 local_bc [28];
  undefined1 local_a0;
  undefined1 local_9f;
  undefined1 local_88 [132];

  cVar3 = FUN_004ec490();
  if (cVar3 == '\x01') {
    puVar5 = local_104;
    for (iVar4 = 0x10; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    local_108 = local_bc;
    local_c4 = local_88;
    local_9f = 0;
    local_a0 = 0;
    do {
      cVar3 = FUN_004ec510();
      puVar2 = local_108;
      bVar1 = false;
      if (cVar3 != '\0') {
        switch(*(undefined4 *)(local_108 + 4)) {
        case 0xd:
          network_game_client_apply_received_update(param_1,&local_108);
          break;
        case 0xf:
          FUN_004aabd0(param_2);
          break;
        case 0x1a:
          FUN_00470810(param_2);
          break;
        case 0x34:
          FUN_004e20b0(param_1);
          break;
        case 0x36:
          network_server_handle_rcon_request();
        }
        if ((puVar2[0x1c] == '\x01') && (puVar2[0x1d] == '\x01')) {
          bVar1 = true;
        }
        else {
          bVar1 = false;
        }
        *(int *)(puVar2 + 0x18) = *(int *)(puVar2 + 0x18) + 1;
        puVar5 = local_104;
        for (iVar4 = 0x10; iVar4 != 0; iVar4 = iVar4 + -1) {
          *puVar5 = 0;
          puVar5 = puVar5 + 1;
        }
        puVar2[0x1c] = 0;
        puVar2[0x1d] = 0;
      }
    } while ((bVar1) && (*(int *)(local_108 + 0x18) <= *(int *)(local_108 + 8)));
  }
  return;
}
#endif
```

## network_client_globals_create.c

```
// network_client_globals_create  (Ghidra: FUN_004dde50; named per this rewrite)
// address 0x4dde50, size 31 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: out/phase4/networking_functions.md: "Allocates the primary network-game globals
// structure (DAT_0071c2d8) via network_session_create and, if successful, clears the DAT_0071c2de flag."
// network_client (0x0071c2d8), network_session_create (0x4d8a80, already rewritten) and
// network_host_handoff_requested (0x0071c2de) all match established names in this module.
```

```
#if 0
Original Ghidra decompilation (0x4dde50):

bool FUN_004dde50(void)

{
  DAT_0071c2d8 = network_session_create();
  if (DAT_0071c2d8 != 0) {
    DAT_0071c2de = 0;
  }
  return DAT_0071c2d8 != 0;
}
#endif
```

## network_client_globals_dispose.c

```
// network_client_globals_dispose  (Ghidra: FUN_004dde70; named per this rewrite)
// address 0x4dde70, size 89 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Tears down the network-game globals
// (DAT_0071c2d8): releases its connection sub-allocation, resets related counters/flags and
// frees the globals themselves." client->update_history (+0xf48) and client->channel (+0xadc)
// match types/networking.h's network_client_globals exactly; network_session_active
// (0x0071c2c2) matches the header's own documented name for that address.
// UNSURE: player_update_history_destroy (outside this batch's range) is called with no visible argument;
// reconstructed as (network_client->update_history), matching player_update_history's own
// "0x4e6b10 destroy" citation in types/networking.h's comment on that type.
```

```
#if 0
Original Ghidra decompilation (0x4dde70):

void FUN_004dde70(void)

{
  int iVar1;

  iVar1 = DAT_0071c2d8;
  if (DAT_0071c2d8 != 0) {
    message_delta_parameters_protocol_dump_to_config_file();
    if (iVar1 != 0) {
      FUN_004e6b10();
      *(undefined4 *)(iVar1 + 0xf48) = 0;
      if (*(int **)(iVar1 + 0xadc) != (int *)0x0) {
        network_channel_delete(*(int **)(iVar1 + 0xadc));
      }
      DAT_0071c2c2 = 0;
    }
    network_stats_summary_log_write();
    DAT_0071c2d8 = 0;
  }
  DAT_0071c2de = 0;
  return;
}
#endif
```

## network_client_handle_server_text_message.c

```
// network_client_handle_server_text_message  (Ghidra: already named)
// address 0x4e5140, size 116 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md ("Client-side handler for a broadcast
// text-message network packet that decodes and prints the message text to the console");
// mirrors network_server_handle_rcon_request.c (this batch), which resolves FUN_004ec590's
// register convention (message pointer in EAX, output buffer in ECX) from its own disassembly.
// register convention: EDX -> message (a decoded network message record whose first dword this
// function checks for a "meaningless" sentinel, exactly as network_server_handle_rcon_request.c
// does with its own in_EAX/in_EDX pair).
//   // blam-cc: EDX -> message
// UNSURE: FUN_004ec590/FUN_004ec670's exact signatures (message-delta protocol, documented in
// types/networking.h as only partly resolved); the decode buffer's shape (a leading char plus 19
// dwords, per Ghidra's own locals) is kept as an opaque byte array sized to match.
```

```
#if 0
Original Ghidra decompilation (0x4e5140), from tools/pack.py 0x4e5140:

void network_client_handle_server_text_message(void)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  undefined4 *in_EDX;
  undefined4 *puVar4;
  char local_54;
  undefined4 local_53 [19];
  undefined1 local_4;

  if (*(int *)*in_EDX == 0) {
    local_54 = '\0';
    puVar4 = local_53;
    for (iVar3 = 0x14; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    cVar1 = FUN_004ec590();
    if (cVar1 != '\0') {
      pcVar2 = &local_54;
      local_4 = 0;
      do {
        cVar1 = *pcVar2;
        pcVar2 = pcVar2 + 1;
      } while (cVar1 != '\0');
      if ((int)pcVar2 - (int)local_53 != 0) {
        chimera__console_out(&DAT_0065efec,&local_54,(int)pcVar2 - (int)local_53);
        return;
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
```

## network_client_identity_tick.c

```
// network_client_identity_tick  (Ghidra: FUN_004db310; renamed, no prior name)
// address 0x4db310, size 422 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Periodic bookkeeping for the local
// client's machine/session identity: expires a stale pending-close flag, then either refreshes
// or re-establishes the client's connection identity depending on whether [acting as host]").
// client+0xee4/+0xee8/+0xeec/+0xef0/+0xef4 match network_client_globals::timer
// (a network_client_timer_record, folded into types/networking.h by the review pass), and
// client+0xef8..+0xf0c is network_client_globals::server_address.
// register convention: __cdecl, single stack parameter `client`.
// // blam-cc: stack -> client
// UNSURE: `network_channel_service_close_if_disconnected`, `network_client_globals_create`, `network_client_globals_dispose` are called with no visible arguments
// and are not in this task's range; declared exactly as shown.
// UNSURE: `datum_get` is called with no visible argument; reconstructed as taking the local
// player's datum index (`local_player_globals[1]`, the same table read in
// network_connection_finalize_join.c), the only value in scope that makes semantic sense here.
// UNSURE: `DAT_0071c2c1` is documented in types/networking.h only as "copied into
// network_game_session::unknown_3ac"; named `network_channel_table_default_flag` here.
// UNSURE: client+0xaf0 (the wcsncpy source) is the same still-unresolved offset already used in
// network_game_settings_packet_send.c (client+0xae0+0x10); accessed the same way, via a raw
// offset, not a named field.
```

```
#if 0
Original Ghidra decompilation (0x4db310):

undefined4 FUN_004db310(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  char *pcVar5;
  undefined8 uVar6;
  LARGE_INTEGER local_34;
  wchar_t local_2c [10];
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  QueryPerformanceCounter(&local_34);
  uVar6 = __allmul(local_34.s.LowPart,local_34.s.HighPart,1000,0);
  uVar1 = __alldiv(uVar6,DAT_006ac8f8,DAT_006ac8fc);
  pcVar5 = (char *)(param_1 + 0xee4);
  if ((*pcVar5 != '\0') && (*(uint *)(param_1 + 0xee8) <= uVar1)) {
    uVar2 = FUN_004dd3f0();
    *pcVar5 = '\0';
    *(undefined4 *)(param_1 + 0xee8) = 0;
    *(undefined1 *)(param_1 + 0xeec) = 1;
    *(uint *)(param_1 + 0xef4) = *(int *)(param_1 + 0xef0) + uVar1;
    return uVar2;
  }
  if ((*(char *)(param_1 + 0xeec) != '\0') &&
     ((*(uint *)(param_1 + 0xef4) <= uVar1 &&
      ((*(byte *)(**(int **)(param_1 + 0xadc) + 0xc) & 0x40) != 0)))) {
    local_18 = *(undefined4 *)(param_1 + 0xef8);
    local_14 = *(undefined4 *)(param_1 + 0xefc);
    local_10 = *(undefined4 *)(param_1 + 0xf00);
    local_c = *(undefined4 *)(param_1 + 0xf04);
    local_8 = *(undefined4 *)(param_1 + 0xf08);
    local_4 = *(undefined4 *)(param_1 + 0xf0c);
    uVar2 = 0xffffffff;
    _wcsncpy(local_2c,(wchar_t *)(param_1 + 0xaf0),8);
    if (DAT_0071c2d4 == 0) {
      if (*(int *)(DAT_0087a478 + 4) != -1) {
        iVar3 = datum_get();
        if (iVar3 != 0) {
          uVar2 = *(undefined4 *)(iVar3 + 0x20);
        }
      }
    }
    else {
      uVar2 = *(undefined4 *)(param_1 + 0xf10);
    }
    FUN_004dde70();
    if (DAT_0071c2d4 != 0) {
      FUN_004dde50();
      *(undefined4 *)(DAT_0071c2d8 + 0xf10) = uVar2;
      return 1;
    }
    DAT_0071c2c1 = 1;
    FUN_004dc8d0(local_2c);
    iVar3 = DAT_0071c2d8;
    for (iVar4 = 0xc; iVar4 != 0; iVar4 = iVar4 + -1) {
      pcVar5[0] = '\0';
      pcVar5[1] = '\0';
      pcVar5[2] = '\0';
      pcVar5[3] = '\0';
      pcVar5 = pcVar5 + 4;
    }
    *(undefined4 *)(param_1 + 0xf10) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0xf10) = uVar2;
    DAT_0071c2c1 = 0;
  }
  return 1;
}
#endif
```

## network_client_rejoin_check.c

```
// network_client_rejoin_check  (Ghidra: FUN_004de390; named per this rewrite)
// address 0x4de390, size 132 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Finds the channel-key entry matching the
// caller's key and parameter and, unless a follow-up check succeeds, flags DAT_0071c2de and
// calls FUN_004aa900 (likely to force a host handoff or disconnect)." The scanned array (client
// treated as short*, +0x669 shorts == byte +0xcd2) matches network_client->session.players[]'s
// machine_index/machine_player_index fields exactly (same evidence as
// network_game_session_reset.c and network_player_entry_add.c). network_session_info_packet_send and
// network_send_join_request_packet are already named by the batch covering 0x4d8a80..0x4d9340.
// The `if (&players[i] == NULL) return;` check is unreachable (client is already known non-NULL
// by that point) and is kept verbatim.
```

```
#if 0
Original Ghidra decompilation (0x4de390):

void FUN_004de390(short param_1)

{
  short sVar1;
  short *psVar2;
  char cVar3;
  uint uVar4;
  int iVar5;
  short *psVar6;

  psVar2 = DAT_0071c2d8;
  if ((DAT_0071c2d8 != (short *)0x0) && (sVar1 = *DAT_0071c2d8, sVar1 != -1)) {
    iVar5 = 0;
    psVar6 = DAT_0071c2d8 + 0x669;
    do {
      cVar3 = network_player_entry_validate();
      if (((cVar3 != '\0') && ((int)(char)*psVar6 == (int)sVar1)) &&
         (*(char *)((int)psVar6 + 1) == param_1)) {
        if (psVar2 + iVar5 * 0x10 + 0x65b == (short *)0x0) {
          return;
        }
        FUN_004d9050(psVar2);
        uVar4 = network_send_join_request_packet((int)DAT_0071c2d8);
        if ((char)uVar4 != '\0') {
          return;
        }
        DAT_0071c2de = 1;
        chat_close();
        return;
      }
      iVar5 = iVar5 + 1;
      psVar6 = psVar6 + 0x10;
    } while (iVar5 < 0x10);
  }
  return;
}
#endif
```

## network_client_send_local_player_updates.c

```
// network_client_send_local_player_updates  (Ghidra: network_client_send_local_player_updates,
// already named)
// address 0x4e77e0, size 174 bytes
// name confidence: 0.7   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md; build_local_player_position_update.c /
// build_local_player_vehicle_update.c (this batch, the two builders this dispatches to, and the
// EBX-out_changed / ESI-plr register convention they document); src/game/player_unit_has_parent.c
// (this batch's sibling call to the same FUN_0056cd10, fixing player_unit_has_parent's real
// signature -- FUN_00477210 here is that same function, called with the iterator's own datum
// handle).
// register convention: none beyond the stack-recognized machine_id; this function itself takes no
// parameters visible in the decompile.
// The on-stack object built from DAT_0087a480 (player_data) and the XOR-with-'iter' constant is
// the inline data_iterator constructor (types/memory.h, signature at +0x0c); DAT_006894a1's exact meaning
// (gates which builder to call) is not otherwise established.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original
```

```
#if 0
Original Ghidra decompilation (0x4e77e0), from tools/pack.py 0x4e77e0:

void network_client_send_local_player_updates(void)

{
  char cVar1;
  int iVar2;
  undefined1 local_11;
  uint local_10;
  undefined2 local_c;
  undefined4 local_8;
  uint local_4;

  local_10 = DAT_0087a480;
  local_4 = DAT_0087a480 ^ 0x69746572;
  local_c = 0;
  local_8 = 0xffffffff;
  iVar2 = data_iterator_next();
  while (iVar2 != 0) {
    if ((*(short *)(iVar2 + 2) == -1) && (*(int *)(iVar2 + 0x34) != -1)) {
      cVar1 = FUN_00477210();
      if ((cVar1 == '\0') || (DAT_006894a1 == '\0')) {
        iVar2 = build_local_player_position_update();
      }
      else {
        iVar2 = build_local_player_vehicle_update(&local_11);
      }
      if (0 < iVar2) {
        network_session_send_to_machine(1,&DAT_00871de0,iVar2,0,0,0,0);
      }
    }
    iVar2 = data_iterator_next();
  }
  return;
}
#endif
```

## network_client_state_dispatch.c

```
// network_client_state_dispatch  (Ghidra: FUN_004d8bb0; renamed, no prior name)
// address 0x4d8bb0, size 66 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md summary ("Dispatches per-frame processing to the
// handler matching the connection's current mode/type field"); the sole caller (network_client_update_dispatch,
// out/halo_decompiled.c around line 143483) loads DAT_0071c2d8 (network_client, per
// types/networking.h) into EAX before the call and reads network_client+0xedc right after it
// returns, confirming in_EAX is network_client here.
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// RESOLVED in the review pass: the switch reads network_client+0xeda as a 16-bit selector with
// five valid values (0..4). types/networking.h declared that field as `pad_eda` (assumed
// padding); this function is the direct evidence that it is a live state value, and the header
// now declares it as `state` with the network_client_state enum.
```

```
#if 0
Original Ghidra decompilation (0x4d8bb0):

undefined1 FUN_004d8bb0(void)

{
  undefined1 uVar1;
  int in_EAX;

  uVar1 = 0;
  switch(*(undefined2 *)(in_EAX + 0xeda)) {
  case 0:
    uVar1 = FUN_004daa20();
    return uVar1;
  case 1:
    uVar1 = FUN_004dab80();
    return uVar1;
  case 2:
    uVar1 = FUN_004daef0();
    break;
  case 3:
    uVar1 = network_game_client_update(in_EAX);
    return uVar1;
  case 4:
    uVar1 = FUN_004db100();
    return uVar1;
  }
  return uVar1;
}
#endif
```

## network_client_timer_default_or_disconnect.c

```
// network_client_timer_default_or_disconnect  (Ghidra: FUN_004d9ce0; renamed, no prior name)
// address 0x4d9ce0, size 51 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Either leaves an already-flagged
// session alone or triggers a network disconnect/cleanup after defaulting the retry-limit
// field"). client+0xedc matches types/networking.h's network_client_globals::unknown_edc
// exactly; network_server+6 bit2 matches network_server_globals::flags's documented
// "bit2 stats logging".
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE: none.
```

```
#if 0
Original Ghidra decompilation (0x4d9ce0):

void FUN_004d9ce0(void)

{
  int in_EAX;

  if (*(short *)(in_EAX + 0xedc) == 0) {
    *(undefined2 *)(in_EAX + 0xedc) = 8;
  }
  if ((DAT_0071c2d4 != 0) && ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) != 0)) {
    return;
  }
  DAT_0071c2de = 1;
  chat_close();
  return;
}
#endif
```

## network_client_timer_schedule.c

```
// network_client_timer_schedule  (Ghidra: FUN_004d9ed0; renamed, no prior name)
// address 0x4d9ed0, size 122 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Schedules a delayed network event/timer
// to fire after a given number of milliseconds"). The five fields written (byte, dword, byte,
// dword, dword at client+0xee4/0xee8/0xeec/0xef0/0xef4) are the first five elements of
// network_client_globals::timer, a network_client_timer_record that the review pass folded
// into types/networking.h (it occupies the first five dwords of the zeroed run at +0xee4).
// register convention: delay in milliseconds in ECX (param_1, per Ghidra's own recovery),
// context/callback value in EDX (param_2), client in ESI (unaff_ESI).
// // blam-cc: ECX -> delay_ms, EDX -> context, ESI -> client
// UNSURE: `network_channel_remote_address_or_default` is `network_channel_remote_address_or_default` (0x4dd390, same address, rewritten
// outside this task's range); called here with no visible argument, reconstructed as taking
// `client`, matching that function's own single-parameter signature.
```

```
#if 0
Original Ghidra decompilation (0x4d9ed0):

void FUN_004d9ed0(int param_1,undefined4 param_2)

{
  int iVar1;
  int unaff_ESI;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  iVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  *(int *)(unaff_ESI + 0xee8) = iVar1 + param_1;
  *(undefined4 *)(unaff_ESI + 0xef0) = param_2;
  *(undefined1 *)(unaff_ESI + 0xee4) = 1;
  *(undefined1 *)(unaff_ESI + 0xeec) = 0;
  *(undefined4 *)(unaff_ESI + 0xef4) = 0;
  FUN_004dd390();
  return;
}
#endif
```

## network_client_update_dispatch.c

```
// network_client_update_dispatch  (Ghidra: FUN_004dded0; named per this rewrite)
// address 0x4dded0, size 224 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Either performs the same host shutdown
// sequence as network_host_shutdown_or_defer (map-state reset and host dispose) when DAT_0071c2de is set, or
// attempts to join/prepare via network_client_state_dispatch/network_client_connect_progress_percent otherwise." network_client_state_dispatch and network_client_connect_progress_percent
// are already named (network_client_state_dispatch, network_client_connect_progress_percent) by
// an earlier batch covering 0x4d8a80..0x4d9050. See network_host_shutdown_or_defer.c for the
// shared shutdown sequence's field evidence.
// UNSURE: DAT_006982e8 is not documented anywhere in types/networking.h; declared generically.
```

```
#if 0
Original Ghidra decompilation (0x4dded0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

char FUN_004dded0(void)

{
  char cVar1;
  int *piVar2;
  char cVar3;

  cVar3 = '\x01';
  if (DAT_0071c2de == '\x01') {
    DAT_00719720 = 0;
    main_menu_music_stop();
    if (DAT_0071c2d4 == (int *)0x0) {
      if (DAT_0071c2d8 == 0) {
        piVar2 = (int *)0x0;
      }
      else {
        piVar2 = (int *)(DAT_0071c2d8 + 0xb14);
      }
    }
    else {
      piVar2 = DAT_0071c2d4 + 2;
    }
    if ((char)piVar2[0xeb] != '\0') {
      chimera__load_ui_map('\x01');
    }
    piVar2[0xeb] = 0;
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
  else {
    cVar1 = FUN_004d8bb0();
    cVar3 = '\0';
    if (cVar1 != '\0') {
      if (*(short *)(DAT_0071c2d8 + 0xedc) == 0) {
        _DAT_006982e8 = FUN_004d8c10();
        return cVar1;
      }
      return '\0';
    }
  }
  return cVar3;
}
#endif
```

## network_connection_endpoint_set.c

```
// network_connection_endpoint_set  (Ghidra: FUN_004d8c50; renamed, no prior name)
// address 0x4d8c50, size 149 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Sets a connection object's remote
// endpoint address fields and (re)allocates its per-endpoint control block").
// register convention: source endpoint data in ESI (unaff_ESI), connection/client object in
// EDI (unaff_EDI). // blam-cc: ESI -> source, EDI -> connection
// UNSURE: both operands are register-only in Ghidra's output (no visible callers in this
// batch), so the exact caller-side setup is unconfirmed; modeled with explicit parameters per
// the task's register-convention rule.
// This writes offsets 0xab4..0xadb of network_client_globals. The review pass folded that
// region into types/networking.h as network_client_globals::connection, a
// network_connection_endpoint. It used to be declared here as
// a local, file-scoped struct covering just the 0x28 bytes this function itself touches,
// immediately preceding the +0xadc channel pointer; not added to types/networking.h.
// network_connection_initiate.c (0x4d8cf0, same task batch) confirms the first five dwords are
// exactly an s_network_address -- it writes the size/port sub-fields at +0x10/+0x12 of this
// struct individually -- so the struct below embeds s_network_address rather than a flat array;
// the same typedef is duplicated there.
```

```
#if 0
Original Ghidra decompilation (0x4d8c50):

undefined4 FUN_004d8c50(void)

{
  undefined4 *puVar1;
  undefined4 *unaff_ESI;
  int unaff_EDI;

  *(undefined1 *)(unaff_EDI + 0xad6) = 0;
  if (*(HGLOBAL *)(unaff_EDI + 0xad8) != (HGLOBAL)0x0) {
    GlobalFree(*(HGLOBAL *)(unaff_EDI + 0xad8));
    *(undefined4 *)(unaff_EDI + 0xad8) = 0;
  }
  *(undefined4 *)(unaff_EDI + 0xab4) = 0;
  *(undefined4 *)(unaff_EDI + 0xab8) = 0;
  *(undefined4 *)(unaff_EDI + 0xabc) = 0;
  *(undefined4 *)(unaff_EDI + 0xac0) = 0;
  *(undefined4 *)(unaff_EDI + 0xac4) = 0;
  *(undefined4 *)(unaff_EDI + 0xac8) = 0;
  *(undefined4 *)(unaff_EDI + 0xacc) = 0;
  *(undefined4 *)(unaff_EDI + 0xad0) = 0;
  *(undefined4 *)(unaff_EDI + 0xad4) = 0;
  *(undefined4 *)(unaff_EDI + 0xad8) = 0;
  *(undefined4 *)(unaff_EDI + 0xab4) = *unaff_ESI;
  *(undefined4 *)(unaff_EDI + 0xab8) = unaff_ESI[1];
  *(undefined4 *)(unaff_EDI + 0xabc) = unaff_ESI[2];
  *(undefined4 *)(unaff_EDI + 0xac0) = unaff_ESI[3];
  *(undefined4 *)(unaff_EDI + 0xac4) = unaff_ESI[4];
  *(undefined4 *)(unaff_EDI + 0xac8) = unaff_ESI[5];
  *(undefined1 *)(unaff_EDI + 0xad6) = 1;
  puVar1 = GlobalAlloc(0,0x264);
  puVar1[1] = 0;
  *puVar1 = 0;
  *(undefined4 **)(unaff_EDI + 0xad8) = puVar1;
  return 1;
}
#endif
```

## network_connection_finalize_join.c

```
// network_connection_finalize_join  (Ghidra: network_connection_finalize_join, already named)
// address 0x4d9960, size 892 bytes
// name confidence: 0.5   rewrite confidence: 0.25 (LOW -- very large function, several
// unresolved sub-regions; see UNSURE notes)
// evidence: out/phase4/networking_functions.md summary ("Finalizes a connection's transition
// into the joined/in-game state, sending the final join packet and arming post-join bookkeeping
// timers"). `connection` is `ushort *` per Ghidra's own recovered signature; every offset below
// is a WORD index doubled to a byte offset, matching this module's other word-indexed functions
// (network_client_state_dispatch.c's client+0xeda, etc). connection+0x56e (byte 0xadc) is
// channel; connection+0x58a (byte 0xb14) is &client->session; connection+0x760 (byte 0xec0) is
// session.unknown_3ac; connection+0x766..0x769 (bytes 0xecc, 0xece, 0xed0, 0xed2) span exactly
// unknown_ecc and unknown_ed0.
// register convention: __cdecl, single stack parameter `connection` (the client).
// // blam-cc: stack -> connection
// UNSURE (major): the player-machine search loop (`*connection` compared against
// `players[i].machine_index`, then `connection + i*0x10` recomputed and re-tested against
// `puVar7[0x669]`) re-derives its own cursor from `connection` directly rather than continuing
// from the byte-0xcd2 base the outer scan used, which does not read as ordinary array indexing.
// Transcribed literally as raw word-pointer arithmetic on `connection`, exactly as Ghidra shows,
// rather than reinterpreted into named player_entry accesses, since the second re-derivation
// cannot be reconciled with a single consistent array base.
// UNSURE: `DAT_0087a478` (used here as `DAT_0087a478 + 4 + index*4`, a flat base address, not a
// small fixed-size table as in network_disconnect_notify_dropped_machines.c) and
// `*(int*)(iVar6+0x34)` off `player_data` are not declared in types/memory.h or
// types/networking.h; named generically. The `sVar9` index this gates on is constrained to
// exactly 0 by its own range check (`-1 < v && v < 1`), consistent with
// network_player_entry::machine_player_index always being 0 on the PC build.
// UNSURE: `data_packet_group_encode_packet`'s output buffer is Ghidra's own `&local_610`, an
// 8-byte `LARGE_INTEGER` reused as a byte buffer -- almost certainly another instance of the
// stack-frame modeling problem seen in network_send_join_request_packet.c (whose analogous call
// uses a genuine 1540-byte buffer). Modeled here with an equivalently generously-sized local
// buffer instead of reusing the 8-byte QPC local, to avoid fabricating an out-of-bounds write.
// UNSURE: several globals (DAT_006b7f98/9a/9e, DAT_0068e684, DAT_0068e680) are not declared
// anywhere in types/networking.h; named generically from their read/write shapes only.
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from &network_challenge_packet_block; the C passed placeholders or dropped the arguments.
```

```
#if 0
Original Ghidra decompilation (0x4d9960):

int __cdecl network_connection_finalize_join(ushort *connection)

{
  ushort *puVar1;
  uint *puVar2;
  uint uVar3;
  char cVar4;
  undefined4 uVar5;
  int iVar6;
  ushort *puVar7;
  uint uVar8;
  short sVar9;
  undefined4 *puVar10;
  undefined4 *puVar11;
  int iVar12;
  undefined8 uVar13;
  LARGE_INTEGER local_610;
  uint local_604;
  undefined4 local_600 [384];

  if (((2 < DAT_0087ac06) && (DAT_006f14b4 != '\0')) && (DAT_006a6140 != (FILE *)0x0)) {
    _fprintf(DAT_006a6140,"%s\t",&DAT_00719879);
  }
  iVar6 = *(int *)(connection + 0x56e);
  connection[0x76c] = 0xffff;
  QueryPerformanceCounter(&local_610);
  uVar13 = __allmul(local_610.s.LowPart,local_610.s.HighPart,1000,0);
  uVar5 = __alldiv(uVar13,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(iVar6 + 4) = uVar5;
  if (DAT_00719720 == 2) {
    *(undefined1 *)(connection + 0x760) = 1;
  }
  else {
    cVar4 = network_game_scenario_load_request((int)(connection + 0x58a));
    if (cVar4 != '\x01') goto LAB_004d9cc4;
  }
  iVar6 = 0;
  puVar7 = connection + 0x669;
  do {
    if ((int)(char)*puVar7 == (uint)*connection) {
      puVar7 = connection + iVar6 * 0x10;
      iVar6 = DAT_0087a480;
      if ((int)(char)puVar7[0x669] == (uint)*connection) goto LAB_004d9a50;
      break;
    }
    iVar6 = iVar6 + 1;
    puVar7 = puVar7 + 0x10;
  } while (iVar6 < 0x10);
  goto LAB_004d9ae0;
  while( true ) {
    uVar8 = FUN_004d98f0((int)*(char *)((int)puVar7 + 0xcd5));
    sVar9 = (short)*(char *)((int)puVar7 + 0xcd3);
    if ((-1 < *(char *)((int)puVar7 + 0xcd3)) && (sVar9 < 1)) {
      puVar2 = (uint *)(DAT_0087a478 + 4 + sVar9 * 4);
      uVar3 = *puVar2;
      if (uVar3 != 0xffffffff) {
        *(undefined2 *)((uVar3 & 0xffff) * 0x200 + 2 + *(int *)(iVar6 + 0x34)) = 0xffff;
        iVar6 = DAT_0087a480;
      }
      *puVar2 = uVar8;
      if (uVar8 != 0xffffffff) {
        *(short *)((uVar8 & 0xffff) * 0x200 + 2 + *(int *)(iVar6 + 0x34)) = sVar9;
      }
    }
    puVar1 = puVar7 + 0x679;
    puVar7 = puVar7 + 0x10;
    if ((int)(char)*puVar1 != (uint)*connection) break;
LAB_004d9a50:
    cVar4 = FUN_004de9f0();
    if (cVar4 == '\0') break;
  }
LAB_004d9ae0:
  iVar6 = *(int *)(connection + 0x56e);
  QueryPerformanceCounter(&local_610);
  uVar13 = __allmul(local_610.s.LowPart,local_610.s.HighPart,1000,0);
  uVar5 = __alldiv(uVar13,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(iVar6 + 4) = uVar5;
  local_610.s.LowPart = 0;
  local_604 = 0x600;
  cVar4 = data_packet_group_encode_packet(&local_610,&local_604,0x1a,1);
  if (cVar4 != '\0') {
    DAT_006b7f98 = ((short)local_604 + 2) * 0x10 | 0xc;
    puVar10 = local_600;
    puVar11 = &DAT_006b7f9a;
    for (uVar8 = (local_604 & 0xffff) >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
      *puVar11 = *puVar10;
      puVar10 = puVar10 + 1;
      puVar11 = puVar11 + 1;
    }
    for (uVar8 = local_604 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
      *(undefined1 *)puVar11 = *(undefined1 *)puVar10;
      puVar10 = (undefined4 *)((int)puVar10 + 1);
      puVar11 = (undefined4 *)((int)puVar11 + 1);
    }
    iVar6 = *(int *)(connection + 0x56e);
    iVar12 = (uint)(DAT_006b7f98 >> 4) * 8;
    if ((*(byte *)(iVar6 + 0xa8c) & 1) == 0) {
      if ((((*(int *)(iVar6 + 0x24) + *(int *)(iVar6 + 0x1c) * -8) - *(int *)(iVar6 + 0x20)) + 1 <
           iVar12 + 1) && (cVar4 = FUN_004ddb60(iVar6,1), cVar4 == '\0')) goto LAB_004d9cc4;
      *(int *)(iVar6 + 0xa80) = *(int *)(iVar6 + 0xa80) + iVar12 + 1;
      bit_stream_write_bits_chunked(1);
      *(undefined1 *)(iVar6 + 0x2c) = 0;
      bit_stream_write_bits_chunked(iVar12);
      *(undefined1 *)(iVar6 + 0x2c) = 0;
    }
    connection[0x76d] = 3;
    connection[0x766] = 0;
    connection[0x767] = 0;
    connection[0x768] = 0;
    connection[0x769] = 0;
    *(undefined1 *)((int)connection + 0xee1) = 0;
    widget_close_all();
    FUN_00470ae0();
    FUN_0045b8b0();
    if ((DAT_00719720 == 2) && ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) == 0)) {
      FUN_004df510(DAT_0071c2d4);
    }
    if (DAT_00718f8c != 0) {
      iVar6 = FUN_00449210();
      uVar8 = 0;
      if ((((DAT_0068e684 != -1) && ((uint)(iVar6 - DAT_0068e684) < 2000)) && (DAT_00718f8c != 1))
         && (uVar8 = (DAT_0068e684 - iVar6) + 2000, 2000 < uVar8)) {
        uVar8 = 2000;
      }
      DAT_0068e680 = uVar8 + 0x6d6 + iVar6;
    }
  }
LAB_004d9cc4:
  return (uint)(connection[0x76d] == 3);
}
#endif
```

## network_connection_initiate.c

```
// network_connection_initiate  (Ghidra: network_connection_initiate, already named)
// address 0x4d8cf0, size 470 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("Begins establishing a connection
// object to a given address/session, substituting the loopback address 127.0.0.1 when the
// target turns out to be the local machine"); connection+0xec4 matches types/networking.h's
// network_client_globals::unknown_ec4 exactly; connection+0xadc matches ::channel; the writes
// to connection+0xac4/0xac6 as two 16-bit halves of a dword whose low half is set to the literal
// 4 (k_network_address_size_ipv4) confirm connection+0xab4 is an s_network_address, which
// upgrades the network_connection_endpoint struct first introduced in
// network_connection_endpoint_set.c; the review pass folded it into types/networking.h.
// register convention: Ghidra's own parameter recovery -- connection in the first cdecl stack
// slot, param_2 (target address, 6 dwords / s_network_address + 1) in the second, param_3
// (9-dword session-info payload) in the third. // blam-cc: cdecl(connection, target, session_info)
// UNSURE: DAT_00718fa4 is not yet declared anywhere in types/networking.h; named here
// network_join_error_code from the -1-sentinel/default-to-7 pattern and from
// out/phase4/networking_functions.md's summary of 0x4d9ce0 ("defaulting the retry-limit field").
// It is read by several other functions in this same task batch (network_join_handshake_tick, network_join_connect_retry_tick,
// network_host_lobby_tick, network_game_client_update, network_host_channel_service_tick) with the same name.
// UNSURE: the boolean this function threads through (bVar11 in Ghidra, `is_local_connection`
// here) is only known by its two visible tests -- whether channel->endpoint exists and whether
// network_channel_attempt_connect(0x96640, 1) (address/mutex registration outside this task's range) succeeds --
// and by the fact the final return is that boolean zero-extended (see below). Named from the
// summary's "target turns out to be the local machine" framing, not independently confirmed.
// UNSURE: `network_address_to_string` and `network_channel_attempt_connect` are called with a Ghidra-elided
// argument and a Ghidra-elided implicit self-pointer respectively (the same register-carryover
// elision documented throughout this codebase). network_address_to_string's argument is
// reconstructed as connection's own endpoint address (the value about to be replaced), the only
// address in scope; network_channel_attempt_connect is shown with two literal integer arguments and needs no
// reconstruction.
// UNSURE: `network_connection_endpoint_set` (network_connection_endpoint_set) is called twice with no visible
// arguments. In the non-loopback branch the manual six-dword copy just above it already
// reproduces exactly what that function would do from `target`, so it is called with
// (target, connection) -- redundant with the manual copy, but both are preserved per the
// no-invented-behaviour rule. In the loopback branch there is no live source value except the
// loopback address just written directly into connection's own memory, so it is called with a
// pointer back into that same memory (a self-copy) rather than with `target` (which would
// overwrite the freshly-built loopback address with the caller's original, empty one). This is
// the best locally-consistent reconstruction, not a confirmed register trace.
// UNSURE: the four separate `inet_addr("127.0.0.1")` calls (rather than one call reused) are
// preserved exactly as Ghidra shows them, in case the compiler genuinely emitted four identical
// calls rather than caching the result.
// UNSURE: the real return value is `CONCAT31((int3)(fn_result >> 8), is_local_connection)` in
// Ghidra, i.e. the low byte of is_local_connection plus three garbage-looking bytes from
// network_connection_endpoint_set's result shifted down. Since that function always returns the
// constant 1, those three bytes are always zero in practice, so this is written as a plain
// zero-extended `(int32_t)is_local_connection` with no behaviour lost.
```

```
#if 0
Original Ghidra decompilation (0x4d8cf0):

int __cdecl network_connection_initiate(int connection,uint *param_2,uint *param_3)

{
  undefined2 uVar1;
  int iVar2;
  short sVar3;
  undefined4 uVar4;
  ulong uVar5;
  ulong uVar6;
  ulong uVar7;
  ulong uVar8;
  int iVar9;
  uint *puVar10;
  bool bVar11;
  undefined8 uVar12;
  LARGE_INTEGER local_8;

  iVar2 = connection;
  *(undefined4 *)(connection + 0xec4) = 1;
  *(undefined4 *)(connection + 0xae0) = 0;
  QueryPerformanceCounter(&local_8);
  uVar12 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar4 = __alldiv(uVar12,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(connection + 0xae8) = 0;
  *(undefined1 *)(connection + 0xaec) = 0;
  *(undefined4 *)(connection + 0xae4) = uVar4;
  puVar10 = (uint *)(connection + 0xaee);
  for (iVar9 = 9; iVar9 != 0; iVar9 = iVar9 + -1) {
    *puVar10 = *param_3;
    param_3 = param_3 + 1;
    puVar10 = puVar10 + 1;
  }
  bVar11 = **(int **)(connection + 0xadc) != 0;
  if ((bVar11) && (**(int **)(connection + 0xadc) != 0)) {
    network_address_to_string();
    sVar3 = FUN_00441f60(0x96640,1);
    if (sVar3 == 0) goto LAB_004d8e78;
    connection._0_1_ = false;
    bVar11 = connection._0_1_;
  }
  else {
LAB_004d8e78:
    if (bVar11) {
      *(undefined2 *)(connection + 0xeda) = 1;
      connection._0_1_ = bVar11;
      goto LAB_004d8dc6;
    }
  }
  connection._0_1_ = bVar11;
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 7;
  }
LAB_004d8dc6:
  puVar10 = (uint *)(iVar2 + 0xab4);
  *puVar10 = 0;
  *(undefined4 *)(iVar2 + 0xab8) = 0;
  *(undefined4 *)(iVar2 + 0xabc) = 0;
  *(undefined4 *)(iVar2 + 0xac0) = 0;
  *(undefined4 *)(iVar2 + 0xac4) = 0;
  *(undefined4 *)(iVar2 + 0xac8) = 0;
  *(undefined4 *)(iVar2 + 0xacc) = 0;
  *(undefined4 *)(iVar2 + 0xad0) = 0;
  *(undefined4 *)(iVar2 + 0xad4) = 0;
  *(undefined4 *)(iVar2 + 0xad8) = 0;
  if ((short)param_2[4] == 0) {
    uVar5 = inet_addr("127.0.0.1");
    uVar6 = inet_addr("127.0.0.1");
    uVar7 = inet_addr("127.0.0.1");
    uVar8 = inet_addr("127.0.0.1");
    uVar1 = (undefined2)DAT_00698208;
    *puVar10 = (uVar5 & 0xff0000 | uVar6 >> 0x10) >> 8 | (uVar7 << 0x10 | uVar8 & 0xff00) << 8;
    *(undefined2 *)(iVar2 + 0xac4) = 4;
    *(undefined2 *)(iVar2 + 0xac6) = uVar1;
    uVar4 = FUN_004d8c50();
    return CONCAT31((int3)((uint)uVar4 >> 8),connection._0_1_);
  }
  *puVar10 = *param_2;
  *(uint *)(iVar2 + 0xab8) = param_2[1];
  *(uint *)(iVar2 + 0xabc) = param_2[2];
  *(uint *)(iVar2 + 0xac0) = param_2[3];
  *(uint *)(iVar2 + 0xac4) = param_2[4];
  *(uint *)(iVar2 + 0xac8) = param_2[5];
  uVar4 = FUN_004d8c50();
  return CONCAT31((int3)((uint)uVar4 >> 8),connection._0_1_);
}
#endif
```

## network_connection_retransmit_if_overdue.c

```
// network_connection_retransmit_if_overdue  (Ghidra: FUN_004d93b0; renamed, no prior name)
// address 0x4d93b0, size 80 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (step 1: objdump -d 0x4d93b0..0x4d93ff -- a stack argument and the helpers' arguments were missing)
// evidence: out/phase4/networking_functions.md summary ("Checks whether a connection's last
// outgoing data has gone unacknowledged past its deadline and, if so, resends it and bumps the
// retry counter"). client+0xad6/+0xab4/+0xad8 match network_client_globals::connection
// (a network_connection_endpoint in types/networking.h since the review pass)
// (ready flag, address.ipv4, control_block); this function additionally shows +0xad2 as a live
// int16 retry counter (inside what those files call the raw `unknown_1c` dword, at its upper
// half) and +0xad4 as a live int16 (their `unknown_20[2]`, matching that field's size exactly).
// register convention: sender address pointer in ECX (in_ECX), client in ESI (unaff_ESI),
// deadline in EDI (unaff_EDI). // blam-cc: ECX -> sender_address, ESI -> client, EDI -> deadline_ms, stack -> remote_time
// UNSURE: none of the three callees are in this task's address range (0x449210 is cseries'
// current-time helper, already declared this way in network_client_connect_progress_percent.c;
// 0x4ed310/0x4ed350 are message-delta code); their signatures are reconstructed from this call
// site alone.
```

```
#if 0
Original Ghidra decompilation (0x4d93b0):

void FUN_004d93b0(void)

{
  short sVar1;
  uint uVar2;
  int *in_ECX;
  int unaff_ESI;
  uint unaff_EDI;

  if ((*(char *)(unaff_ESI + 0xad6) != '\0') && (*(int *)(unaff_ESI + 0xab4) == *in_ECX)) {
    uVar2 = FUN_00449210();
    if (unaff_EDI <= uVar2) {
      *(short *)(unaff_ESI + 0xad2) = *(short *)(unaff_ESI + 0xad2) + 1;
      FUN_004ed310(*(undefined4 *)(unaff_ESI + 0xad8));
      sVar1 = FUN_004ed350();
      *(short *)(unaff_ESI + 0xad4) = sVar1 << 1;
    }
  }
  return;
}
#endif
```

## network_connection_send_keepalive.c

```
// network_connection_send_keepalive  (Ghidra: FUN_004d9400; renamed, no prior name)
// address 0x4d9400, size 178 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Periodically sends a keepalive packet
// on an idle connection once more than three seconds have elapsed since the last send").
// Confirms and refines network_client_globals::connection, shared with
// network_connection_endpoint_set.c / network_connection_initiate.c /
// network_connection_retransmit_if_overdue.c: what those files call the raw `unknown_18` dword
// (0xacc) is a live millisecond timestamp here, and what network_connection_retransmit_if_overdue.c
// calls `unknown_1c_lo` (0xad0) is a live int16 counter this function increments.
// register convention: the client pointer arrives in ESI (unaff_ESI). // blam-cc: ESI -> client
// UNSURE: `network_prepare_challenge_packet` is called here with no visible argument at all
// (unlike its call site in network_session_info_packet_send.c, which has an 8-dword local built
// immediately beforehand); modeled here as taking no visible source, since none is ever
// constructed in this function's own body.
// UNSURE: `network_channel_reliable_pool_store` (0x4dcdb0, outside this task's range -- types/networking.h names it as
// the reliable-retransmit-pool "store" step) is reconstructed purely from this call site's
// literal argument shapes.
```

```
#if 0
Original Ghidra decompilation (0x4d9400):

void FUN_004d9400(void)

{
  DWORD DVar1;
  int iVar2;
  int unaff_ESI;
  undefined8 uVar3;
  undefined1 local_9;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  DVar1 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
  if ((*(char *)(unaff_ESI + 0xad6) == '\x01') && (3000 < DVar1 - *(int *)(unaff_ESI + 0xacc))) {
    local_8.s.LowPart = DVar1;
    iVar2 = network_prepare_challenge_packet();
    if (iVar2 != 0) {
      local_9 = 0;
      if ((*(byte *)(*(int *)(unaff_ESI + 0xadc) + 0xa8c) & 1) == 0) {
        FUN_004dcdb0(*(int *)(unaff_ESI + 0xadc),iVar2,&local_9,0);
      }
      *(short *)(unaff_ESI + 0xad0) = *(short *)(unaff_ESI + 0xad0) + 1;
      *(DWORD *)(unaff_ESI + 0xacc) = DVar1;
    }
  }
  return;
}
#endif
```

## network_disconnect_notify_dropped_machines.c

```
// network_disconnect_notify_dropped_machines  (Ghidra: FUN_004d9340; renamed, no prior name)
// address 0x4d9340, size 104 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Displays a disconnect-notification
// error message for each machine that dropped from the session"). client+0xee0 matches
// types/networking.h's network_client_globals::unknown_ee0 exactly.
// register convention: the client pointer arrives in EBX (unaff_EBX). // blam-cc: EBX -> client
// UNSURE (major, preserved exactly): as decompiled, the while loop can run at most once --
// after the first `display_error` call, the loop-continuation test can only ever re-select -1
// (it requires `player_index < 0`, but player_index was just set to 0 to enter the loop body at
// all), so despite the summary's "for each machine" framing this cannot iterate over more than
// one machine as written. This is the same class of decompiler information-loss already flagged
// in player_data_iterator_advance.c (player_data_iterator_advance) -- almost certainly a lost per-iteration
// update to the real index variable -- and is transcribed literally rather than reconstructed,
// per the task's no-invented-behaviour rule.
// UNSURE: `DAT_00697e78` (a one-shot "already notified" style gate) and the table read through
// `DAT_0087a478 + 4` are not declared anywhere in types/networking.h; named/typed generically
// below. `DAT_0087a478` sits immediately before types/networking.h's `player_data`
// (0x0087a480, owned by the game/objects modules), so it is very likely a distinct, smaller
// table rather than a stray offset into player_data itself.
// UNSURE: `display_error`'s signature is reconstructed purely from this one call site's literal
// argument shapes (int, int, bool, bool); not cross-checked against any other caller.
```

```
#if 0
Original Ghidra decompilation (0x4d9340):

void FUN_004d9340(void)

{
  short sVar1;
  int iVar2;
  int unaff_EBX;
  int player_index;

  if (DAT_00697e78 == '\0') {
    if (*(char *)(unaff_EBX + 0xee0) == '\0') {
      player_index = -1;
      if (*(int *)(DAT_0087a478 + 4) != -1) {
        player_index = 0;
      }
      sVar1 = (short)player_index;
      while (sVar1 != -1) {
        display_error(8,player_index,'\x01','\0');
        iVar2 = -1;
        if ((*(int *)(DAT_0087a478 + 4) != -1) && ((short)player_index < 0)) {
          iVar2 = 0;
        }
        player_index = iVar2;
        sVar1 = (short)iVar2;
      }
    }
    *(undefined1 *)(unaff_EBX + 0xee0) = 1;
  }
  return;
}
#endif
```

## network_game_client_connect_to_address.c

```
// network_game_client_connect_to_address  (Ghidra: network_game_client_connect_to_address,
// already named)
// address 0x4dc790, size 314 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Parses a user-entered address[:port] string,
// resolves it to a numeric address, and kicks off the connect handshake." Confirmed against
// objdump -d -M intel bin/halo.exe at 0x4dc790..0x4dc8c9: the address-string argument is EAX
// (saved into EBX at entry, in_EAX in Ghidra's decompile); the four inet_addr() calls per branch
// are the same "call an accessor four times with the same argument and reassemble a byte-swapped
// 32-bit value via mask/shift" pattern already documented in
// network_channel_get_remote_address.c (there for gamespy_array_length/gt2GetLocalIP), reused verbatim
// here for inet_addr. The result, together with size = k_network_address_size_ipv4 and a port
// (parsed after ':' when present via network_address_parse_port... actually _atol directly here,
// or network_game_socket_port when address_string has no ':'), is written to a local
// s_network_address; disassembly at 0x4dc8b5 (`lea ecx,[esp+0xc]`) passes that record's address
// as the hidden ECX argument to network_client_begin_connect, which the callee's own body confirms by testing
// address->ipv4 != 0 and address->port != 0 (in_ECX and *(short*)(in_ECX+0x12) in its decompile).
// register convention: EAX -> address_string; player_name is the one ordinary cdecl stack
// parameter. blam-cc: EAX -> address_string, stack -> player_name
```

```
#if 0
Original Ghidra decompilation (0x4dc790):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void network_game_client_connect_to_address(undefined4 param_1)

{
  char cVar1;
  char *in_EAX;
  char *pcVar2;
  undefined1 *puVar3;
  char local_100 [256];

  pcVar2 = in_EAX;
  do {
    cVar1 = *pcVar2;
    pcVar2[(int)(local_100 + -(int)in_EAX)] = cVar1;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  puVar3 = (undefined1 *)FUN_006257e0(local_100,0x3a);
  if (puVar3 == (undefined1 *)0x0) {
    inet_addr(in_EAX);
    inet_addr(in_EAX);
    inet_addr(in_EAX);
    inet_addr(in_EAX);
  }
  else {
    *puVar3 = 0;
    inet_addr(local_100);
    inet_addr(local_100);
    inet_addr(local_100);
    inet_addr(local_100);
    _atol(puVar3 + 1);
  }
  if (in_EAX == (char *)0x0) {
    _DAT_006b2f28 = 0;
  }
  else {
    FUN_00557990();
  }
  FUN_004dc8d0(param_1);
  return;
}
#endif
```

## network_game_client_update.c

```
// network_game_client_update  (Ghidra: network_game_client_update, already named)
// address 0x4daf80, size 377 bytes
// name confidence: 0.55   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Main per-tick network update for an
// actively-connected client: services the channel, processes incoming messages, flushes
// outgoing data, and (when a debug flag is set) periodically logs ping/latency/timing"). Reuses
// the network_client_globals::connection fields already named in
// network_connection_send_keepalive.c (message_count@0xad0, retry_count@0xad2, unknown_20@0xad4,
// control_block@0xad8) and the channel-flags bit layout confirmed in network_host_update_tick.c.
// register convention: __cdecl, single stack parameter `client`.
// // blam-cc: stack -> client
// UNSURE: `ui_network_wait_timeout_start`, `network_channel_service_light`, `network_channel_service_retransmit_only` are called with no visible arguments
// and are not in this task's range; declared exactly as shown.
// UNSURE: `DAT_0071c2dc` is documented only loosely in types/networking.h ("shortens the
// disconnect timeout when clear"); named `network_disconnect_timeout_flag` here.
// UNSURE: `DAT_00710306` (a debug-print gate byte) and `DAT_0071c2c4` (the last-printed ping
// sample cache) are not declared in types/networking.h; named generically.
```

```
#if 0
Original Ghidra decompilation (0x4daf80):

undefined1 __cdecl network_game_client_update(int param_1)

{
  int iVar1;
  int *piVar2;
  char cVar3;
  bool bVar4;
  undefined1 uVar5;
  undefined4 uVar6;
  int iVar7;
  uint uVar8;

  uVar8 = (*(int **)(param_1 + 0xadc))[0x2a3];
  if (((((~(byte)(uVar8 >> 4) & 1) != 0) && ((uVar8 & 6) != 0)) &&
      (iVar1 = **(int **)(param_1 + 0xadc), iVar1 != 0)) && ((*(byte *)(iVar1 + 0xc) & 1) != 0)) {
    if ((DAT_0071c2d4 == 0) || (DAT_0071c2dc != '\0')) {
      if ((uVar8 >> 5 & 1) != 0) {
        FUN_0049c810();
      }
      *(byte *)(param_1 + 0xee1) = (byte)(uVar8 >> 5) & 1;
    }
    cVar3 = FUN_004dd240(0);
    if (DAT_00719720 == 2) {
      network_channel_record_timestamp(*(int *)(param_1 + 0xadc));
    }
    if (cVar3 != '\0') {
      bVar4 = network_game_process_incoming_messages(param_1);
      uVar5 = 0;
      if (bVar4) {
        uVar5 = FUN_004dd330();
      }
      goto LAB_004db07a;
    }
    piVar2 = *(int **)(param_1 + 0xadc);
    if ((((~(byte)((uint)piVar2[0x2a3] >> 4) & 1) != 0) && ((*(byte *)(piVar2 + 0x2a3) & 6) != 0))
       && ((*piVar2 != 0 && (uVar5 = 0, (*(byte *)(*piVar2 + 0xc) & 1) != 0)))) goto LAB_004db07a;
  }
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 4;
  }
  uVar5 = 0;
LAB_004db07a:
  FUN_004d9400();
  if (((DAT_00710306 == '\x01') &&
      (uVar8 = (uint)*(ushort *)(param_1 + 0xad0), uVar8 != DAT_0071c2c4)) && (uVar8 % 10 == 0)) {
    DAT_0071c2c4 = uVar8;
    console_print_error_va
              ("current ping time[%d]  samples received[%d]  samples sent[%d]\n",
               *(undefined2 *)(param_1 + 0xad4),*(undefined2 *)(param_1 + 0xad2),uVar8);
    iVar1 = **(int **)(param_1 + 0xad8);
    uVar6 = FUN_004ed350();
    iVar7 = FUN_00449210();
    console_print_error_va
              ("current time delta[%d]  latency[%d]  server time[%d]\n",iVar1,uVar6,iVar7 + iVar1);
  }
  return uVar5;
}
#endif
```

## network_game_record_message_send.c

```
// network_game_record_message_send  (Ghidra: FUN_004da130; renamed, no prior name)
// address 0x4da130, size 288 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Encodes a large (up to 0x600-byte)
// outgoing network-game message (type id 0x12) from an 8-dword source record and queues it for
// transmission"). This function has zero callers anywhere in the binary (out/functions.json
// callers=0); rewritten anyway per the task instructions, since it is not listed as
// misattributed/library code in out/phase4/networking_types_notes.md.
// register convention: client in the sole cdecl stack parameter; the 8-dword source record
// arrives in EDX (in_EDX). // blam-cc: EDX -> source, stack -> client
// The 8-dword source record is copied to a local (byte 30 of it is forced from 0xff to 0), encoded
// into a separate 0x600-byte buffer by data_packet_group_encode_packet (EAX = that buffer, EBX =
// network_game_messages_group 0x6994f8, stack = record, &size, 0x12, 1), and the encoded bytes
// are wrapped by network_message_block_build. Return value is only the low byte (0/1).
// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from record; the C passed placeholders or dropped the arguments.

// VERIFIED against disassembly 0x4da130..0x4da250 (2026-09-30): FIXED: encode call takes EAX = separate 0x600 output buffer and EBX = network_game_messages_group (was missing); block_build now wraps that encoded buffer, not the 32-byte record copy; stream-space/flush/write sequence matches
```

```
#if 0
Original Ghidra decompilation (0x4da130):

uint FUN_004da130(int param_1)

{
  int iVar1;
  uint uVar2;
  undefined3 uVar3;
  undefined3 extraout_var;
  int iVar4;
  undefined4 *in_EDX;
  undefined4 *puVar5;
  int iVar6;
  ushort *local_624;
  undefined4 local_620 [7];
  char local_602;

  puVar5 = local_620;
  for (iVar4 = 8; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar5 = *in_EDX;
    in_EDX = in_EDX + 1;
    puVar5 = puVar5 + 1;
  }
  if (local_602 == -1) {
    local_602 = '\0';
  }
  local_624 = (ushort *)0x600;
  uVar2 = data_packet_group_encode_packet(local_620,&local_624,0x12,1);
  if ((char)uVar2 != '\0') {
    local_624 = (ushort *)FUN_00440350(local_624);
    uVar2 = 0;
    if (local_624 != (ushort *)0x0) {
      iVar1 = *(int *)(param_1 + 0xadc);
      uVar3 = (undefined3)((uint)param_1 >> 8);
      iVar6 = (uint)(*local_624 >> 4) * 8;
      iVar4 = iVar6 + 1;
      if ((*(byte *)(iVar1 + 0xa8c) & 1) == 0) {
        if (((*(int *)(iVar1 + 0x24) + *(int *)(iVar1 + 0x1c) * -8) - *(int *)(iVar1 + 0x20)) + 1 <
            iVar4) {
          uVar2 = FUN_004ddb60(iVar1,1);
          if ((char)uVar2 == '\0') {
            return uVar2 & 0xffffff00;
          }
        }
        *(int *)(iVar1 + 0xa80) = *(int *)(iVar1 + 0xa80) + iVar4;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar6);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
        uVar3 = extraout_var;
      }
      return CONCAT31(uVar3,1);
    }
  }
  return uVar2 & 0xffffff00;
}
#endif
```

## network_host_channel_service_tick.c

```
// network_host_channel_service_tick  (Ghidra: FUN_004db100; renamed, no prior name)
// address 0x4db100, size 120 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Per-tick network update used while
// acting as host/server: services the channel, drains incoming messages, updates per-machine
// state, and sends a periodic ping"). Shares the channel-flags bit4 ("not dead") test with
// network_host_update_tick.c.
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE: none beyond the shared cluster-wide notes on network_channel_service's argument.
```

```
#if 0
Original Ghidra decompilation (0x4db100):

char FUN_004db100(void)

{
  char cVar1;
  bool bVar2;
  int in_EAX;

  cVar1 = FUN_004dd110(0);
  if (cVar1 != '\0') {
    bVar2 = network_game_process_incoming_messages(in_EAX);
    if ((bVar2) && (cVar1 = FUN_004db310(), cVar1 != '\0')) {
      FUN_004d9400();
      return cVar1;
    }
  }
  if ((~(byte)(*(uint *)(*(int *)(in_EAX + 0xadc) + 0xa8c) >> 4) & 1) != 0) {
    return '\0';
  }
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 4;
  }
  return '\0';
}
#endif
```

## network_host_lobby_tick.c

```
// network_host_lobby_tick  (Ghidra: FUN_004daef0; renamed, no prior name)
// address 0x4daef0, size 138 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md: "Per-tick update for a hosted game: broadcasts
// presence, services the network channel, and drains incoming messages while the host's socket
// is healthy." It is case 2 of network_client_state_dispatch (0x4d8bb0), which sits between the
// two join-side states (0, 1) and the in-game states (3 = network_game_client_update, 4 =
// network_host_channel_service_tick), and it is the only caller of
// network_host_presence_broadcast_tick (0x4dadb0, the once-a-second LAN announcement), so this
// is the state a host runs in while sitting in the pre-game lobby.
// This function is a near-twin of network_host_channel_service_tick (0x4db100): same channel
// guard, same network_channel_service/network_game_process_incoming_messages pair, same
// network_join_error_code fallback. It differs only by the extra presence broadcast and by
// requiring the endpoint to be live before doing any work.
// register convention: the client pointer arrives in ESI (unaff_ESI). // blam-cc: ESI -> client
// evidence for the call shapes, from objdump -d -M intel (0x4daef0):
//   4daf1c  mov eax,esi / call 0x4dadb0        -> broadcast tick takes the client in EAX
//   4daf23  mov edi,[esi+0xadc]                -> channel in EDI for the service call
//   4daf29  push 0 / mov eax,0x3a98 / call 0x4dd110
//                                              -> network_channel_service(EDI=channel,
//                                                 EAX=0x3a98=15000 ms, stack arg 0)
//   4daf3c  push esi / call 0x4db180           -> process_incoming_messages(client) on the stack
//   4daf63  cmp WORD PTR ds:0x718fa4,0xffff    -> the error code is a 16-bit field, not 32-bit
```

```
#if 0
Original Ghidra decompilation (0x4daef0):

undefined1 FUN_004daef0(void)

{
  int *piVar1;
  char cVar2;
  bool bVar3;
  int unaff_ESI;

  piVar1 = *(int **)(unaff_ESI + 0xadc);
  if (((((~(byte)((uint)piVar1[0x2a3] >> 4) & 1) != 0) && ((*(byte *)(piVar1 + 0x2a3) & 6) != 0)) &&
      (*piVar1 != 0)) && ((*(byte *)(*piVar1 + 0xc) & 1) != 0)) {
    FUN_004dadb0();
    cVar2 = FUN_004dd110(0);
    if ((cVar2 != '\0') && (bVar3 = network_game_process_incoming_messages(unaff_ESI), bVar3)) {
      return 1;
    }
  }
  if (((~(byte)(*(uint *)(*(int *)(unaff_ESI + 0xadc) + 0xa8c) >> 4) & 1) == 0) &&
     (DAT_00718fa4 == -1)) {
    DAT_00718fa4 = 4;
  }
  return 0;
}
#endif
```

## network_host_presence_broadcast_tick.c

```
// network_host_presence_broadcast_tick  (Ghidra: FUN_004dadb0; renamed, no prior name)
// address 0x4dadb0, size 313 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("Periodically (every second) builds and
// queues a short broadcast-style message carrying a global status string, consistent with a
// hosted-game LAN presence announcement"). client+0xed4 matches types/networking.h's
// network_client_globals::unknown_ed4 exactly.
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE: `cache_file_request_map(1)`'s real signature/argument meaning is not resolved here;
// declared generically from this call site.

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from challenge; the C passed placeholders or dropped the arguments.
```

```
#if 0
Original Ghidra decompilation (0x4dadb0):

void FUN_004dadb0(void)

{
  char cVar1;
  int in_EAX;
  int iVar2;
  undefined4 *puVar3;
  int iVar4;
  undefined8 uVar5;
  LARGE_INTEGER local_110;
  char local_108;
  undefined4 local_107;

  QueryPerformanceCounter(&local_110);
  uVar5 = __allmul(local_110.s.LowPart,local_110.s.HighPart,1000,0);
  iVar2 = __alldiv(uVar5,DAT_006ac8f8,DAT_006ac8fc);
  if (*(int *)(in_EAX + 0xed4) + 1000 < iVar2) {
    *(int *)(in_EAX + 0xed4) = iVar2;
    cVar1 = cache_file_request_map(1);
    if (cVar1 != '\0') {
      local_108 = '\0';
      puVar3 = &local_107;
      for (iVar2 = 0x3f; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar3 = 0;
        puVar3 = puVar3 + 1;
      }
      *(undefined2 *)puVar3 = 0;
      *(undefined1 *)((int)puVar3 + 2) = 0;
      _strncpy(&local_108,&DAT_00719879,0x100);
      local_110.s.LowPart = network_prepare_challenge_packet();
      if ((ushort *)local_110.s.LowPart != (ushort *)0x0) {
        iVar2 = *(int *)(in_EAX + 0xadc);
        iVar4 = (uint)(*(ushort *)local_110.s.LowPart >> 4) * 8;
        if ((*(byte *)(iVar2 + 0xa8c) & 1) == 0) {
          if ((((*(int *)(iVar2 + 0x24) + *(int *)(iVar2 + 0x1c) * -8) - *(int *)(iVar2 + 0x20)) + 1
               < iVar4 + 1) && (cVar1 = FUN_004ddb60(iVar2,1), cVar1 == '\0')) {
            return;
          }
          *(int *)(iVar2 + 0xa80) = *(int *)(iVar2 + 0xa80) + iVar4 + 1;
          bit_stream_write_bits_chunked(1);
          *(undefined1 *)(iVar2 + 0x2c) = 0;
          bit_stream_write_bits_chunked(iVar4);
          *(undefined1 *)(iVar2 + 0x2c) = 0;
        }
      }
    }
  }
}
#endif
```

## network_join_connect_retry_tick.c

```
// network_join_connect_retry_tick  (Ghidra: FUN_004dab80; renamed, no prior name)
// address 0x4dab80, size 550 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Per-tick state machine that updates
// the join UI status text/timeout and retries the connect attempt while the client channel is
// not yet fully connected"). channel+0xa8c (piVar2[0x2a3]) is network_channel::flags;
// channel->endpoint (*piVar2) leads into network_receive_queue's own +0xc flags byte and +0xe
// last_error, both per types/networking.h. client+0xae0/+0xae4/+0xae8/+0xaec match
// network_client_globals::connect_attempt (a network_connection_attempt_state in
// types/networking.h since the review pass), shared with network_connection_initiate.c /
// chimera__on_connect.c / network_client_connect_progress_percent.c.
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE: `network_signal_quality_glyph` and `network_receive_queue_close_socket` are called with no visible arguments and are not in
// this task's range; declared as taking no arguments, exactly as shown.
// UNSURE: the `-0x18` last_error comparison and the `-1 < (char)endpoint->flags` sign test have
// no further-resolved meaning beyond their evidenced role here.
```

```
#if 0
Original Ghidra decompilation (0x4dab80):

bool FUN_004dab80(void)

{
  int iVar1;
  int *piVar2;
  char cVar3;
  bool bVar4;
  int in_EAX;
  int iVar5;
  undefined4 uVar6;
  undefined8 uVar7;
  LARGE_INTEGER local_c;

  QueryPerformanceCounter(&local_c);
  uVar7 = __allmul(local_c.s.LowPart,local_c.s.HighPart,1000,0);
  iVar5 = __alldiv(uVar7,DAT_006ac8f8,DAT_006ac8fc);
  iVar1 = *(int *)(in_EAX + 0xadc);
  QueryPerformanceCounter(&local_c);
  uVar7 = __allmul(local_c.s.LowPart,local_c.s.HighPart,1000,0);
  uVar6 = __alldiv(uVar7,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(iVar1 + 4) = uVar6;
  piVar2 = *(int **)(in_EAX + 0xadc);
  if ((((piVar2[0x2a3] & 6U) == 0) || (*piVar2 == 0)) || ((*(byte *)(*piVar2 + 0xc) & 1) == 0)) {
    if (*(short *)(*piVar2 + 0xe) == -0x18) {
      if (DAT_00718fa4 == -1) {
        DAT_00718fa4 = FUN_00440610();
        return false;
      }
    }
    else {
      if (-1 < *(char *)(*piVar2 + 0xc)) {
        if (*(int *)(in_EAX + 0xae0) == 0) {
          if ((*(byte *)(in_EAX + 0xede) & 4) == 0) {
            if (3000 < (uint)((iVar5 + *(int *)(in_EAX + 0xae8) * -3000) - *(int *)(in_EAX + 0xae4))
               ) {
              FUN_004db4c0();
            }
            goto LAB_004dad7d;
          }
        }
        else {
          if ((uint)(iVar5 - *(int *)(in_EAX + 0xae4)) <= DAT_006894ac) goto LAB_004dad7d;
          *(undefined4 *)(in_EAX + 0xae0) = 0;
        }
        FUN_00442040();
      }
      if (DAT_00718fa4 == -1) {
        DAT_00718fa4 = 3;
      }
    }
    return false;
  }
  *(undefined4 *)(in_EAX + 0xae0) = 0;
  if (*(char *)(in_EAX + 0xaec) == '\0') {
    *(undefined4 *)(in_EAX + 0xae8) = 0;
    FUN_00496a80("Loading");
    DAT_00718f90 = 0;
    if (DAT_00719720 == 2) {
      if (DAT_00718f8c != 1) {
        if ((DAT_00718f8c != 2) && (DAT_00718f8c == 4)) {
          DAT_0068e688 = 0xffffffff;
        }
        DAT_00718f8c = 8;
        *(undefined1 *)(in_EAX + 0xaec) = 1;
        goto LAB_004dad7d;
      }
    }
    else if ((DAT_00718f8c != 1) && (DAT_00718f8c != 2)) {
      if (DAT_00718f8c == 4) {
        DAT_0068e688 = 0xffffffff;
        *(undefined1 *)(in_EAX + 0xaec) = 1;
        goto LAB_004dad7d;
      }
      DAT_00718f8c = 7;
    }
    *(undefined1 *)(in_EAX + 0xaec) = 1;
  }
LAB_004dad7d:
  cVar3 = FUN_004dd110(0);
  bVar4 = false;
  if (cVar3 != '\0') {
    bVar4 = network_game_process_incoming_messages(in_EAX);
  }
  return bVar4;
}
#endif
```

## network_join_handshake_tick.c

```
// network_join_handshake_tick  (Ghidra: FUN_004daa20; renamed, no prior name)
// address 0x4daa20, size 348 bytes
// name confidence: 0.4   rewrite confidence: 0.25 (LOW -- unresolved stack-frame overlap; see
// UNSURE notes)
// evidence: out/phase4/networking_functions.md summary ("Per-tick update that drives the
// client's join-a-server handshake: services the transport, and once a specific waiting
// sub-state is reached builds and sends the address/name connect request"). network_server+4
// matches network_server_globals::unknown_004 ("tested against 0 and 2 by host_dispose" per
// types/networking.h); network_server+0x9fc matches ::password exactly.
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE (major): by strict byte-offset arithmetic on Ghidra's own declared locals, the
// loopback address this function computes (into what Ghidra calls local_170/local_160/local_15e)
// does NOT overlap the 6-dword buffer (local_134) actually passed as
// network_connection_initiate's `target` argument -- that buffer is separately, fully zeroed and
// never touched again, meaning `target` reaches network_connection_initiate as all-zero,
// triggering that function's own loopback substitution. The computed loopback address therefore
// appears to go unread by anything in this function's own body. The only call between its
// computation and network_connection_initiate is `network_debug_fill_canary_buffer()` (zero visible arguments, not
// in this task's range), so it is the most likely real consumer; not confirmed. Preserved
// exactly (the writes are made, in the same order, whether or not they are the "real" target).
// FIXED in the review pass: `network_channel_service` is network_channel_service and its three arguments
// are all recoverable from the disassembly (see the call site comment below).
// UNSURE: `network_debug_fill_canary_buffer` is still called with no reconstructed argument.
// UNSURE: `local_a` (written 1) falls, by the same byte-offset arithmetic, inside the tail of
// the local_134 array rather than in the 9-dword session_info range read by
// network_connection_initiate; preserved as a write into the same combined scratch buffer used
// for local_134, at its own computed offset, rather than folded into session_info.
```

```
#if 0
Original Ghidra decompilation (0x4daa20):

uint FUN_004daa20(void)

{
  int iVar1;
  bool bVar2;
  int in_EAX;
  undefined4 uVar3;
  uint uVar4;
  int iVar5;
  uint *puVar6;
  bool bVar7;
  undefined8 uVar8;
  LARGE_INTEGER local_17c;
  int local_170;
  undefined2 local_160;
  undefined2 local_15e;
  undefined4 local_158;
  undefined2 local_146;
  uint local_134 [74];
  undefined2 local_a;

  QueryPerformanceCounter(&local_17c);
  iVar5 = *(int *)(in_EAX + 0xadc);
  QueryPerformanceCounter(&local_17c);
  uVar8 = __allmul(local_17c.s.LowPart,local_17c.s.HighPart,1000,0);
  uVar3 = __alldiv(uVar8,DAT_006ac8f8,DAT_006ac8fc);
  iVar1 = DAT_0071c2d4;
  bVar7 = DAT_0071c2d4 == 0;
  *(undefined4 *)(iVar5 + 4) = uVar3;
  bVar2 = true;
  if (bVar7) {
    uVar4 = FUN_004dd110(0);
    if ((char)uVar4 == '\0') {
LAB_004dab24:
      if (DAT_00718fa4 != -1) {
        return uVar4;
      }
      DAT_00718fa4 = 7;
      return uVar4;
    }
    bVar2 = network_game_process_incoming_messages(in_EAX);
    if (bVar2) {
      FUN_004d9400();
    }
  }
  else if (*(short *)(iVar1 + 4) == 1) {
    local_134[1] = 0;
    local_134[2] = 0;
    local_134[3] = 0;
    local_134[0] = 0;
    puVar6 = local_134 + 4;
    for (iVar5 = 0x48; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    local_170 = DAT_006869b0;
    if (DAT_006869b0 == 0) {
      local_170 = 0x7f000001;
    }
    local_15e = (undefined2)DAT_00698208;
    local_160 = 4;
    local_a = 1;
    _wcsncpy((wchar_t *)((int)&local_158 + 2),(wchar_t *)(iVar1 + 0x9fc),8);
    local_146 = 0;
    FUN_004e0790();
    uVar4 = network_connection_initiate(in_EAX,local_134,&local_158);
    if ((char)uVar4 == '\0') goto LAB_004dab24;
  }
  return (uint)bVar2;
}
#endif
```

## network_join_hostname_resolved_callback.c

```
// network_join_hostname_resolved_callback  (Ghidra: FUN_004ba270; named per this rewrite)
// address 0x4ba270, size 174 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Completion callback for the
// asynchronous hostname-resolve used when joining a server by name: stores the resolved
// address/port, or falls back to a direct connect-by-string path").
// register convention: Ghidra recognizes all three as ordinary stack parameters (cc=unknown,
// no in_/unaff_ registers), so they are kept as plain parameters with no blam-cc remapping.
// UNSURE: `hostent` is read only by raw offset (+0x02 a 16-bit field, +0x04 a 32-bit field),
// matching a Winsock `hostent`-shaped async-resolve buffer (h_addrtype-ish / first address
// dword), but nothing in this batch confirms the real type; declared as a raw byte pointer.
// UNSURE: the resolved address gt2AddressToString builds into the stack buffer is never read again in
// this function -- the actual connect call below it always uses one of two persistent globals
// instead. Preserved exactly as decompiled; this may be dead code in the retail build or a
// buffer Ghidra mismatched, not "improved" per the task's no-invented-behaviour rule.
// UNSURE: `network_join_error_code`, `split_screen_quit_prompt_string` and
// `network_join_target_address` are named from behavior only; gt2NetworkToHostShort/gt2AddressToString are
// unnamed network glue functions outside this batch's address range.
```

```
#if 0
Original Ghidra decompilation (0x4ba270):

void FUN_004ba270(int param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined2 *puVar3;
  undefined2 local_18;
  undefined4 local_16;
  undefined4 local_12;
  undefined4 local_e;
  undefined4 local_a;
  undefined4 local_6;

  local_16 = 0;
  local_12 = 0;
  local_e = 0;
  local_a = 0;
  local_6 = 0;
  local_18 = 0;
  if (param_3 != 0) {
    uVar1 = *(undefined4 *)(param_3 + 4);
    uVar2 = FUN_006148a0(*(undefined2 *)(param_3 + 2));
    FUN_006148b0(uVar1,uVar2,&local_18);
  }
  if (param_1 == 0) {
    if (DAT_00719454 == '\0') {
      puVar3 = (undefined2 *)&DAT_00660c34;
    }
    else {
      puVar3 = &DAT_00719458;
    }
    network_game_client_connect_to_address(puVar3);
    DAT_00719458 = 0;
    DAT_00719454 = 0;
    return;
  }
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 0x2b;
  }
  DAT_0071973c = 0;
  DAT_00719754._0_2_ = 0xffff;
  DAT_00719754._3_1_ = 1;
  return;
}
#endif
```

## network_join_request_resolve_host.c

```
// network_join_request_resolve_host  (Ghidra: FUN_004ba320; named per
// out/phase2/results/networking_01.json / symbols/review_queue.txt)
// address 0x4ba320, size 819 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Resolves the host/IP for a pending
// 'join server' request and either connects immediately or kicks off an asynchronous hostname
// resolution"); out/phase2/results/networking_01.json evidence ("reads a pending connect
// request (DAT_00719450), retrieves hostname/IP fields via SBServerGetPublicAddress/00617640/00617650,
// compares packed address bytes, formats \"%s:%d\" via sprintf, and either connects directly
// via network_game_client_connect_to_address or starts an async resolve with NNBeginNegotiationWithSocket passing callbacks FUN_0044ad80
// and network_join_hostname_resolved_callback (0x4ba270), setting the connect state
// DAT_00718f8c=4"); the packed-address comparison mirrors
// src/networking/network_channel_get_remote_address.c's identical gamespy_array_length-based idiom;
// DAT_0071946c is named `master_server_query_engine` per
// src/networking/master_server_process_pending_requests.c (same address, same GameSpy handle
// role, reused here); DAT_0068e680/4/8, DAT_00718f8c/90 and DAT_006b2f28/68 are the
// interface module's "loading screen" globals per out/phase2/results/interface_01.json
// ("resets every loading-screen related global ... to their inactive sentinel values").
// register convention: Ghidra recognizes no parameters at all (`signature: undefined
// FUN_004ba320(void)`, no in_/unaff_ registers in the body) -- this function reads everything
// from globals.
// UNSURE: disassembly of this function's `call 0x4dc790` (network_game_client_connect_to_
// address) shows `lea eax,[esp+...]` loading the address of the freshly sprintf'd "host:port"
// string immediately before the call, with only the `&network_join_target_address` stack
// argument otherwise visible -- i.e. the callee also takes an EAX argument. The one other call
// site in this batch's range (network_join_hostname_resolved_callback, 0x4ba270, already
// committed) shows the identical `lea eax,[esp+...]; push <stack arg>; call 0x4dc790` shape,
// but that file's own extern declares only the stack parameter. This file declares the fuller,
// two-argument prototype directly (each function file's extern declarations are independent,
// per this project's one-file-per-function model), rather than editing the already-committed
// file; out of scope for this batch, but worth a follow-up pass over 0x4ba270's declaration.
// UNSURE: the three `local_5c` byte-copy loops (inlined strcpy of SBServerGetPublicAddress/00617640/
// 00617040's results) write into a local buffer that is never read again anywhere in the
// function -- apparently dead, but preserved for fidelity exactly as
// network_join_hostname_resolved_callback.c preserves its own similarly dead local buffer.
// UNSURE: network_channels_open (0x441300, already committed) is declared `void` there because
// none of its own callers used a return value; this function's Ghidra decompile treats its
// result as a real `uint` fed into `return uVar5 & 0xffffff00`, but that value is genuinely
// just whatever network_channels_open's own last callee happened to leave in EAX (confirmed by
// disassembly: no explicit return-value write before its final `ret`). Preserved as `0` in the
// two return paths that depend on it, since the already-committed `void` signature leaves no
// value to read.
```

```
#if 0
Original Ghidra decompilation (0x4ba320):

uint FUN_004ba320(void)

{
  char cVar1;
  bool bVar2;
  char *pcVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  int iVar11;
  undefined4 uVar12;
  undefined4 uVar13;
  undefined4 uVar14;
  undefined4 uVar15;
  size_t sVar16;
  char local_78 [21];
  uint local_63;
  char local_5c [28];
  char local_40 [64];

  local_78[1] = '\0';
  local_78[2] = '\0';
  local_78[3] = '\0';
  local_78[4] = '\0';
  local_78[5] = '\0';
  local_78[6] = '\0';
  local_78[7] = '\0';
  local_78[8] = '\0';
  local_78[9] = '\0';
  local_78[10] = '\0';
  local_78[0xb] = '\0';
  local_78[0xc] = '\0';
  local_78[0xd] = '\0';
  local_78[0xe] = '\0';
  local_78[0xf] = '\0';
  local_78[0x10] = '\0';
  local_78[0x11] = '\0';
  local_78[0x12] = '\0';
  local_78[0x13] = '\0';
  local_78[0x14] = '\0';
  local_78[0] = '\0';
  local_63 = 0;
  bVar2 = false;
  FUN_00617600(DAT_00719450);
  pcVar3 = (char *)FUN_006175e0(DAT_00719450);
  if (pcVar3 != (char *)0x0) {
    iVar6 = -(int)pcVar3;
    do {
      cVar1 = *pcVar3;
      pcVar3[(int)(local_5c + iVar6)] = cVar1;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
  }
  FUN_00617650(DAT_00719450);
  pcVar3 = (char *)FUN_00617640(DAT_00719450);
  if (pcVar3 != (char *)0x0) {
    iVar6 = -(int)pcVar3;
    do {
      cVar1 = *pcVar3;
      pcVar3[(int)(local_5c + iVar6)] = cVar1;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
  }
  pcVar3 = (char *)FUN_00617040(DAT_0071946c);
  if (pcVar3 != (char *)0x0) {
    iVar6 = -(int)pcVar3;
    do {
      cVar1 = *pcVar3;
      pcVar3[(int)(local_5c + iVar6)] = cVar1;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
  }
  uVar4 = FUN_00617600(DAT_00719450);
  uVar5 = FUN_006175f0(DAT_00719450);
  FUN_006175f0(DAT_00719450);
  iVar6 = FUN_006175f0(DAT_00719450);
  uVar7 = FUN_006175f0(DAT_00719450);
  FUN_00617060(DAT_0071946c);
  iVar8 = FUN_00617060(DAT_0071946c);
  uVar9 = FUN_00617060(DAT_0071946c);
  uVar10 = FUN_00617060(DAT_0071946c);
  iVar11 = FUN_00617620(DAT_00719450);
  if ((iVar11 == 0) ||
     (((uVar5 & 0xff0000) >> 8 | (iVar6 << 0x10 | uVar7 & 0xff00) << 8) !=
      ((iVar8 << 0x10 | uVar9 & 0xff00) << 8 | uVar10 >> 8 & 0xff00))) {
    iVar6 = FUN_00617630(DAT_00719450);
    if (iVar6 == 0) {
      bVar2 = true;
    }
    else {
      sVar16 = 0x18;
      pcVar3 = (char *)FUN_006175e0(DAT_00719450);
      _strncpy(local_78,pcVar3,sVar16);
      local_63 = local_63 & 0xffffff;
    }
  }
  else {
    sVar16 = 0x18;
    pcVar3 = (char *)FUN_00617640(DAT_00719450);
    _strncpy(local_78,pcVar3,sVar16);
    local_63 = local_63 & 0xffffff;
    uVar4 = FUN_00617650(DAT_00719450);
  }
  uVar5 = network_channels_open();
  if (DAT_006869be == '\0') {
    DAT_00719450 = 0;
    return uVar5 & 0xffffff00;
  }
  if (!bVar2) {
    DAT_0068e680 = 0xffffffff;
    DAT_0068e684 = 0xffffffff;
    DAT_0068e688 = 0xffffffff;
    DAT_00718f8c = 0;
    DAT_00718f90 = 0;
    _DAT_006b2f28 = 0;
    _DAT_006b2f68 = 0;
    _sprintf(local_40,"%s:%d",local_78,uVar4 & 0xffff);
    uVar5 = network_game_client_connect_to_address(&DAT_00719458);
    DAT_00719458 = 0;
    DAT_00719454 = 0;
    DAT_00719450 = 0;
    return uVar5;
  }
  uVar12 = FUN_006175f0(DAT_006f14c4);
  uVar13 = FUN_006175e0(DAT_00719450);
  uVar14 = FUN_00617600(DAT_00719450);
  uVar15 = FUN_004403b0(10000);
  FUN_00616f50(DAT_0071946c,uVar13,uVar14,uVar15);
  uVar5 = FUN_00614f30(uVar12,uVar15,1,FUN_0044ad80,FUN_004ba270,0);
  DAT_00719450 = 0;
  if (uVar5 == 0) {
    DAT_0068e688 = uVar15;
    DAT_0068e680 = 0xffffffff;
    DAT_0068e684 = 0xffffffff;
    DAT_00718f90 = 0;
    _DAT_006b2f28 = 0;
    _DAT_006b2f68 = 0;
    DAT_00718f8c = 4;
    return 0xffffff01;
  }
  return uVar5 & 0xffffff00;
}
#endif
```

## network_join_status_text_update.c

```
// network_join_status_text_update  (Ghidra: FUN_004db4c0; renamed, no prior name)
// address 0x4db4c0, size 359 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Updates the on-screen join/connect
// status text ('Loading', 'Connecting', or an animated 'Connecting...') and the associated UI
// state machine based on an integer mode selector").
// register convention: the mode selector arrives in EAX (in_EAX), the client pointer in ESI
// (unaff_ESI). // blam-cc: EAX -> mode, ESI -> client
// UNSURE: none.
```

```
#if 0
Original Ghidra decompilation (0x4db4c0):

void FUN_004db4c0(void)

{
  int in_EAX;
  size_t _Count;
  int unaff_ESI;
  char local_14 [20];

  if (in_EAX == 0) {
    *(undefined4 *)(unaff_ESI + 0xae8) = 0;
    FUN_00496a80("Connecting");
    DAT_00718f90 = 0;
    DAT_00718f8c = 5;
  }
  else if (in_EAX == 1) {
    local_14[0] = '\0';
    local_14[1] = '\0';
    local_14[2] = '\0';
    local_14[3] = '\0';
    local_14[0xc] = '\0';
    local_14[0xd] = '\0';
    local_14[0xe] = '\0';
    local_14[0xf] = '\0';
    local_14[4] = '\0';
    local_14[5] = '\0';
    local_14[6] = '\0';
    local_14[7] = '\0';
    _Count = *(int *)(unaff_ESI + 0xae8) + 1;
    local_14[8] = '\0';
    local_14[9] = '\0';
    local_14[10] = '\0';
    local_14[0xb] = '\0';
    local_14[0x10] = 0;
    *(size_t *)(unaff_ESI + 0xae8) = _Count;
    if ((int)_Count < 0) {
      _Count = 0;
    }
    else if (0x10 < (int)_Count) {
      _Count = 0x10;
    }
    _strncpy(local_14,s__________________00697e7c,_Count);
    FUN_00496a80("Connecting%s",local_14);
    DAT_00718f90 = *(undefined4 *)(unaff_ESI + 0xae8);
    if ((DAT_00718f8c != 1) && (DAT_00718f8c != 2)) {
      if (DAT_00718f8c == 4) {
        DAT_0068e688 = 0xffffffff;
        return;
      }
      DAT_00718f8c = 6;
      return;
    }
  }
  else {
    *(undefined4 *)(unaff_ESI + 0xae8) = 0;
    FUN_00496a80("Loading");
    DAT_00718f90 = 0;
    if (DAT_00719720 == 2) {
      if (DAT_00718f8c != 1) {
        if ((DAT_00718f8c != 2) && (DAT_00718f8c == 4)) {
          DAT_0068e688 = 0xffffffff;
        }
        DAT_00718f8c = 8;
        return;
      }
    }
    else if ((DAT_00718f8c != 1) && (DAT_00718f8c != 2)) {
      if (DAT_00718f8c == 4) {
        DAT_0068e688 = 0xffffffff;
        DAT_00718f90 = 0;
        return;
      }
      DAT_00718f8c = 7;
      return;
    }
  }
  return;
}
#endif
```

## network_player_join_finalize.c

```
// network_player_join_finalize  (Ghidra: network_player_join_finalize, already named)
// address 0x4d9e30, size 156 bytes
// name confidence: 0.55   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Creates the game-object datum for a
// player once their connection has fully joined, marking it as the local player when
// applicable"). `unaff_EDI` is a word-indexed client pointer (word 0x58a / byte 0xb14 is
// &client->session, matching the other word-indexed functions in this cluster); word 0x76d
// (byte 0xeda) is state; `in_EAX+0x1f` matches network_player_entry::slot_index.
// register convention: client in EDI (unaff_EDI); entry (a network_player_entry*) in EAX
// (unaff_EAX). Confirmed by objdump: `mov esi,eax` at 0x4d9e32, before the first call, saves it
// across both the validate and add calls; the SAME (unmodified) value is read again at
// 0x4d9e62 (`movsx ecx,[esi+0x1f]`, entry->slot_index) after the add call returns -- so entry is
// a genuine caller-supplied pointer, not (as this file previously guessed)
// network_player_entry_add's own return value.
// blam-cc: EDI -> client, EAX -> entry
// FIXED (register inputs, objdump): EAX carries entry (read at 0x4d9e32, mov esi,eax); it was
// missing, and the body had synthesized `entry` from network_player_entry_add's return instead.
// objdump also shows network_player_entry_validate (0x4d9e36) and network_player_entry_add
// (0x4d9e4c) both take this same entry pointer via EAX (validate: EAX only; add: EAX -> entry,
// stack -> &client->session), and that network_player_entry_add itself only ever returns a
// plain bool in AL (never re-read as a pointer) -- settling how those two calls' arguments are
// passed.
// The row created by network_player_entry_add is players[entry->slot_index]; its own slot_index feeds
// player_data_iterator_advance (stack), whose result is the player handle used by
// game_set_local_player (ECX handle, SI player index), datum_new_at_index_with_salt (EAX handle,
// EDX = update_client_queues) and update_server_queue_create_entry (EAX handle).

// VERIFIED against disassembly 0x4d9e30..0x4d9ecc (2026-09-30): FIXED: player_data_iterator_advance result (handle) is now kept and passed to game_set_local_player (ECX, SI), datum_new_at_index_with_salt (EAX, EDX = update_client_queues) and update_server_queue_create_entry (EAX); network_channel_key_open gets its row (EAX); row indexed by signed slot_index
```

```
#if 0
Original Ghidra decompilation (0x4d9e30):

char network_player_join_finalize(void)

{
  char cVar1;
  char cVar2;
  int in_EAX;
  ushort *unaff_EDI;

  cVar2 = FUN_004de9f0();
  if (cVar2 == '\0') {
    return '\0';
  }
  cVar2 = FUN_004de4e0(unaff_EDI + 0x58a);
  if ((cVar2 != '\0') && (unaff_EDI[0x76d] == 3)) {
    cVar1 = *(char *)(in_EAX + 0x1f);
    cVar2 = FUN_004de870();
    if (cVar2 == '\0') {
      return '\0';
    }
    FUN_004d98f0((int)*(char *)((int)unaff_EDI + cVar1 * 0x20 + 0xcd5));
    if ((int)(char)unaff_EDI[cVar1 * 0x10 + 0x669] == (uint)*unaff_EDI) {
      game_set_local_player();
    }
    datum_new_at_index_with_salt();
    if (DAT_0071c2d4 != 0) {
      FUN_00472c90();
    }
  }
  return cVar2;
}
#endif
```

## network_send_join_request_packet.c

```
// network_send_join_request_packet  (Ghidra: network_send_join_request_packet, already named)
// address 0x4d9220, size 279 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Encodes and queues an outgoing
// join-request packet (packet type 0x1e) on the given connection"); shares the channel+0xa8c/
// +0x24/+0x1c/+0x20/+0xa80/+0x2c raw-offset idiom already used (and left unresolved) in
// network_session_info_packet_send.c and src/game/game_engine_send_team_allegiance_message.c.
// register convention: __cdecl, single stack parameter `connection` (channel at +0xadc matches
// network_client_globals, so this is the client). // blam-cc: stack -> connection
// The join request is encoded exactly like network_game_record_message_send: data_packet_group_encode_packet
// (EAX = a 0x600-byte output buffer, EBX = network_game_messages_group 0x6994f8, stack = payload
// pointer, &capacity, 0x1e, 1), then network_message_block_build wraps the encoded bytes, then the
// record is appended to the channel's outgoing bit stream. The payload argument is the address of
// an uninitialized 4-byte local (this message type carries no body). Return value is AL only.

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from record; the C passed placeholders or dropped the arguments.

// VERIFIED against disassembly 0x4d9220..0x4d9337 (2026-09-30): FIXED: encoder called with EAX = separate 0x600 buffer, EBX = network_game_messages_group and a payload pointer; block_build wraps the encoded buffer; same send sequence as network_game_record_message_send
```

```
#if 0
Original Ghidra decompilation (0x4d9220):

uint __cdecl network_send_join_request_packet(int connection)

{
  int iVar1;
  int iVar2;
  char extraout_AL;
  uint uVar3;
  undefined3 uVar4;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  int iVar5;
  char local_60a;
  ushort *local_608;
  undefined1 local_604 [1540];

  if ((DAT_0071c2d4 == 0) || ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) == 0)) {
    DAT_0071c2de = 1;
    chat_close();
  }
  local_608 = (ushort *)0x600;
  uVar3 = data_packet_group_encode_packet(local_604,&local_608,0x1e,1);
  if ((char)uVar3 != '\0') {
    local_608 = (ushort *)FUN_00440350(local_608);
    uVar3 = 0;
    if (local_608 != (ushort *)0x0) {
      iVar2 = *(int *)(connection + 0xadc);
      uVar4 = (undefined3)((uint)local_608 >> 8);
      iVar5 = (uint)(*local_608 >> 4) * 8;
      iVar1 = iVar5 + 1;
      local_60a = '\x01';
      if ((*(byte *)(iVar2 + 0xa8c) & 1) == 0) {
        if (((*(int *)(iVar2 + 0x24) + *(int *)(iVar2 + 0x1c) * -8) - *(int *)(iVar2 + 0x20)) + 1 <
            iVar1) {
          FUN_004ddb60(iVar2,1);
          uVar4 = extraout_var;
          local_60a = extraout_AL;
          if (extraout_AL == '\0') goto LAB_004d931e;
        }
        *(int *)(iVar2 + 0xa80) = *(int *)(iVar2 + 0xa80) + iVar1;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar2 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar5);
        *(undefined1 *)(iVar2 + 0x2c) = 0;
        uVar4 = extraout_var_00;
      }
LAB_004d931e:
      return CONCAT31(uVar4,local_60a);
    }
  }
  return uVar3 & 0xffffff00;
}
#endif
```

## network_session_create.c

```
// network_session_create  (Ghidra: network_session_create, already named)
// address 0x4d8a80, size 236 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/networking.h "network_client_globals (0x4d8a80 network_session_create,
// 0x4d8b70 destroy)" -- every DAT_ global in this function is network_client_storage
// (0x00872de0) plus a fixed delta, matched field by field against that struct; the
// out/phase4/networking_types_notes.md "network_client_globals (0xf4c)" paragraph confirms the
// same deltas and confirms +0xb14 is the session network_game_session_reset receives.
// register convention: none (void); Ghidra shows no incoming registers.
// UNSURE: the final 13-dword zero of unknown_f14 runs unconditionally, even on the
// network_channel_new(2) failure path where the local base pointer has just been set to 0 --
// Ghidra's "puVar2 + 0xf14" byte arithmetic on a 0 puVar2 means that path would write
// through raw address 0xf14. Preserved exactly (see the task's no-invented-behaviour rule);
// this is presumably a real, effectively unreachable bug in the retail binary (network_channel_new
// failing here needs a socket-creation failure), not a decompiler artifact -- the two loops and
// the intervening if/else are still standard structured control flow either way.
// UNSURE: the 12-dword zero loop starting at unknown_ee4 writes one dword past the declared
// 11-element array, into unknown_f10; the very next statement then overwrites unknown_f10 with
// -1. Both are preserved exactly, in the original order.
// UNSURE: the calls to network_game_session_reset and network_session_destroy carry no
// visible arguments in the Ghidra output (their real parameters arrive in EDX/EAX respectively,
// per those functions' own signatures); network_client_storage (the client argument) is what
// every other evidence in this file's neighbourhood says is live in that register at the call
// site, so it is passed explicitly here.
```

```
#if 0
Original Ghidra decompilation (0x4d8a80):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined * network_session_create(void)

{
  int iVar1;
  undefined *puVar2;
  undefined4 *puVar3;

  puVar2 = &DAT_00872de0;
  DAT_0071c2c2 = 1;
  message_delta_protocol_initialize();
  _DAT_00873d28 = GlobalAlloc(0,0x2c);
  *_DAT_00873d28 = 0;
  _DAT_00873d28[1] = 0;
  _DAT_00873d28[2] = 0;
  _DAT_00873d28[3] = 0;
  _DAT_00873d28[4] = 0;
  _DAT_00873d28[5] = 0;
  _DAT_00873d28[6] = 0;
  _DAT_00873d28[7] = 0;
  _DAT_00873d28[8] = 0;
  _DAT_00873d28[9] = 0;
  _DAT_00873d28[10] = 0;
  _DAT_008738bc = network_channel_new(2);
  if (_DAT_008738bc == (int *)0x0) {
    network_session_destroy();
    puVar2 = (undefined *)0x0;
  }
  else {
    network_channel_table_initialize();
    DAT_00873cbe = DAT_00873cbe & 0xfff9;
    _DAT_00872de0 = 0xffff;
    _DAT_00873cba = 0;
    _DAT_00873cbc = 0;
    _DAT_00873ca8 = 0;
    _DAT_00873cac = 0;
    _DAT_00873cb0 = 0;
    DAT_00873cc1 = 0;
    _DAT_00873cb8 = 0xffff;
    DAT_00873cc0 = 0;
    puVar3 = &DAT_00873cc4;
    for (iVar1 = 0xc; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    _DAT_00873cf0 = 0xffffffff;
  }
  puVar3 = (undefined4 *)(puVar2 + 0xf14);
  for (iVar1 = 0xd; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  network_stats_summary_log_open();
  return puVar2;
}
#endif
```

## network_session_destroy.c

```
// network_session_destroy  (Ghidra: network_session_destroy, already named)
// address 0x4d8b70, size 64 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/networking.h "network_client_globals (0x4d8a80 network_session_create,
// 0x4d8b70 destroy)"; the two offsets this function writes (+0xf48, +0xadc) are exactly
// update_history and channel, matching out/phase4/networking_types_notes.md's
// "network_session_destroy (0x4d8b70) frees the +0xf48 list and deletes the +0xadc channel".
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE: none.
```

```
#if 0
Original Ghidra decompilation (0x4d8b70):

void network_session_destroy(void)

{
  int in_EAX;

  message_delta_parameters_protocol_dump_to_config_file();
  if (in_EAX != 0) {
    FUN_004e6b10();
    *(undefined4 *)(in_EAX + 0xf48) = 0;
    if (*(int **)(in_EAX + 0xadc) != (int *)0x0) {
      network_channel_delete(*(int **)(in_EAX + 0xadc));
    }
    DAT_0071c2c2 = 0;
  }
  network_stats_summary_log_write();
  return;
}
#endif
```

## network_session_info_packet_send.c

```
// network_session_info_packet_send  (Ghidra: FUN_004d9050; renamed, no prior name)
// address 0x4d9050, size 292 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Encodes and queues a session-info
// packet for the connection, when its connection-mode field indicates that one is needed").
// register convention: the 8-dword source record arrives in EAX (in_EAX); client is the sole
// cdecl stack parameter. // blam-cc: EAX -> source, stack -> client
// Dispatch on client->state (jump table at 0x4d9174): 0 and 1 return 0, 2 builds message type
// 0x10, 3 builds 0x1d, 4 builds 0x23, and any other state returns 1 without sending. The 8-dword
// source record is copied to a local whose address is the payload (EDX) of
// network_prepare_challenge_packet (EAX = message type); the resulting record is appended to the
// channel's outgoing bit stream exactly as in network_game_record_message_send. Return is AL only.

// VERIFIED against disassembly 0x4d9050..0x4d9174 (2026-09-30): FIXED: state 3 builds message type 0x1d (was 0x23); states 0/1 -> 0, >4 -> 1 per the jump table at 0x4d9174; send sequence compared
```

```
#if 0
Original Ghidra decompilation (0x4d9050):

char FUN_004d9050(int param_1)

{
  ushort uVar1;
  char cVar2;
  undefined4 *in_EAX;
  ushort *puVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 local_20 [8];

  cVar2 = '\x01';
  switch(*(undefined2 *)(param_1 + 0xeda)) {
  case 0:
  case 1:
    break;
  case 2:
    puVar6 = local_20;
    for (iVar4 = 8; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = *in_EAX;
      in_EAX = in_EAX + 1;
      puVar6 = puVar6 + 1;
    }
    puVar3 = (ushort *)network_prepare_challenge_packet();
    if (puVar3 != (ushort *)0x0) {
      uVar1 = *puVar3;
      iVar4 = *(int *)(param_1 + 0xadc);
      goto LAB_004d90e5;
    }
    break;
  case 3:
    goto LAB_004d90b8;
  case 4:
LAB_004d90b8:
    puVar6 = local_20;
    for (iVar4 = 8; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = *in_EAX;
      in_EAX = in_EAX + 1;
      puVar6 = puVar6 + 1;
    }
    puVar3 = (ushort *)network_prepare_challenge_packet();
    if (puVar3 != (ushort *)0x0) {
      uVar1 = *puVar3;
      iVar4 = *(int *)(param_1 + 0xadc);
LAB_004d90e5:
      iVar5 = (uint)(uVar1 >> 4) * 8;
      cVar2 = '\x01';
      if (((*(byte *)(iVar4 + 0xa8c) & 1) == 0) &&
         ((iVar5 + 1 <=
           ((*(int *)(iVar4 + 0x24) + *(int *)(iVar4 + 0x1c) * -8) - *(int *)(iVar4 + 0x20)) + 1 ||
          (cVar2 = FUN_004ddb60(iVar4,1), cVar2 != '\0')))) {
        *(int *)(iVar4 + 0xa80) = *(int *)(iVar4 + 0xa80) + iVar5 + 1;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar4 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar5);
        *(undefined1 *)(iVar4 + 0x2c) = 0;
      }
      return cVar2;
    }
    break;
  default:
    goto switchD_004d906f_default;
  }
  cVar2 = '\0';
switchD_004d906f_default:
  return cVar2;
}
#endif
```

## network_session_player_join_notify.c

```
// network_session_player_join_notify  (Ghidra: FUN_004d9700; renamed, no prior name)
// address 0x4d9700, size 218 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Records a newly-joined player's index
// into the active session state and queues a notification packet announcing the join"). Shares
// the challenge/send idiom (channel+0xa8c/+0x24/+0x1c/+0x20/+0xa80/+0x2c) with the other
// functions in this cluster. `in_EAX+0x76d` (word index, byte 0xeda) is client->state;
// `in_EAX+0x56e` (byte 0xadc) is client->channel; server+0x3ac and client+0xeb8 both land at
// session-relative offset 0x3a4 (2 bytes into types/networking.h's opaque
// network_game_session::unknown_3a2[10]), confirming both containers embed the same session.
// register convention: client (word-indexed base) in EAX, a 2-dword source record in ECX.
// // blam-cc: EAX -> client, ECX -> source
// UNSURE: `client->unknown_000` is declared in types/networking.h as initialized to 0xffff by
// network_session_create; here it is overwritten with the validated 0..15 player index, so it
// appears to double as a "pending/just-joined player index" scratch field. Not renamed (header
// not editable here).
// UNSURE: source[0] (the dword copied into session.unknown_3a2+2) and source[1]'s low word (the
// validated player index) have no further-resolved meaning beyond their evidenced roles here.

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from challenge; the C passed placeholders or dropped the arguments.
```

```
#if 0
Original Ghidra decompilation (0x4d9700):

void FUN_004d9700(void)

{
  int iVar1;
  short sVar2;
  undefined4 uVar3;
  int iVar4;
  char cVar5;
  short *in_EAX;
  ushort *puVar6;
  undefined4 *in_ECX;
  int iVar7;
  bool bVar8;

  sVar2 = *(short *)(in_ECX + 1);
  if ((-1 < sVar2) && (sVar2 < 0x10)) {
    *in_EAX = sVar2;
    iVar1 = DAT_0071c2d4;
    bVar8 = DAT_0071c2d4 != 0;
    in_EAX[0x76d] = 2;
    uVar3 = *in_ECX;
    if (bVar8) {
      *(undefined4 *)(iVar1 + 0x3ac) = uVar3;
    }
    if (DAT_0071c2d8 != 0) {
      *(undefined4 *)(DAT_0071c2d8 + 0xeb8) = uVar3;
    }
    puVar6 = (ushort *)network_prepare_challenge_packet();
    if (puVar6 != (ushort *)0x0) {
      iVar4 = *(int *)(in_EAX + 0x56e);
      iVar7 = (uint)(*puVar6 >> 4) * 8;
      iVar1 = iVar7 + 1;
      if (((*(byte *)(iVar4 + 0xa8c) & 1) == 0) &&
         ((iVar1 <= ((*(int *)(iVar4 + 0x24) + *(int *)(iVar4 + 0x1c) * -8) - *(int *)(iVar4 + 0x20)
                    ) + 1 || (cVar5 = FUN_004ddb60(iVar4,1), cVar5 != '\0')))) {
        *(int *)(iVar4 + 0xa80) = *(int *)(iVar4 + 0xa80) + iVar1;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar4 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar7);
        *(undefined1 *)(iVar4 + 0x2c) = 0;
      }
    }
  }
  return;
}
#endif
```

## network_session_player_table_index_apply.c

```
// network_session_player_table_index_apply  (Ghidra: FUN_004d9190; renamed, no prior name)
// address 0x4d9190, size 139 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Finds the player slot matching a given
// machine id and records its assigned table index (e.g. team or score-table slot) for that
// player"); the loop cursor starts at param_1+0xcd2 and steps by 0x20 (network_player_entry's
// stride) comparing its first two bytes against candidate+0x1c/+0x1d, which are exactly
// network_player_entry::machine_index/machine_player_index (0x1c/0x1d) -- so
// param_1+0xcd2-0x1c = param_1+0xcb6 = &client->session.players[0], confirming param_1 is
// network_client_globals* (client->session.players sits at client+0xb14+0x1a2 = client+0xcb6).
// register convention: Ghidra's own recovered stack parameters (param_1 = client, param_2 = the
// table index to record); `unaff_EBX` (the candidate machine_index/machine_player_index source)
// is an elided register argument, reconstructed as a third parameter.
// blam-cc: stack -> client, table_index; EBX -> candidate
// UNSURE: `candidate`'s type is unknown -- it is read only at byte offsets +0x1c/+0x1d, the same
// two offsets network_player_entry uses for the same two fields, but nothing pins its overall
// layout or size; kept as a raw `const uint8_t *` rather than invented as a named struct.
// UNSURE: `network_player_entry_validate` (types/networking.h's own validator for network_player_entry, per
// out/phase4/networking_types_notes.md) is called with no visible argument in Ghidra's output
// (its result only appears via `extraout_AL`, a classic sign the argument-setup instruction was
// elided); reconstructed as taking the current loop candidate entry
// (`&client->session.players[i]`), the only pointer in scope that matches its documented
// "requires +0x1d in 0..0, +0x1c in 0..15" signature.
// UNSURE: player_data's growable-array base pointer is read as `*(int *)(player_data + 0x34)`;
// types/memory.h's data_array does not (yet) name an offset-0x34 field, so it is read via a raw
// offset on `(uint8_t *)player_data` rather than guessed into a new header field.
// `param_1 + 0xec0` is client->session.unknown_3ac (0xb14 + 0x3ac session offset = 0xec0).
```

```
#if 0
Original Ghidra decompilation (0x4d9190):

undefined4 FUN_004d9190(int param_1,int param_2)

{
  char extraout_AL;
  uint uVar1;
  int unaff_EBX;
  int iVar2;
  char *pcVar3;

  iVar2 = 0;
  pcVar3 = (char *)(param_1 + 0xcd2);
  while( true ) {
    FUN_004de9f0();
    if (((extraout_AL != '\0') && (*pcVar3 == *(char *)(unaff_EBX + 0x1c))) &&
       (pcVar3[1] == *(char *)(unaff_EBX + 0x1d))) break;
    iVar2 = iVar2 + 1;
    pcVar3 = pcVar3 + 0x20;
    if (0xf < iVar2) {
      return 0;
    }
  }
  uVar1 = FUN_004d98f0((int)*(char *)(iVar2 * 0x20 + 0xcd5 + param_1));
  if (((*(char *)(param_1 + 0xec0) != '\0') && (uVar1 != 0)) &&
     ((uVar1 != 0xffffffff && (param_2 != -1)))) {
    *(int *)((uVar1 & 0xffff) * 0x200 + 0xd0 + *(int *)(DAT_0087a480 + 0x34)) = param_2;
  }
  return 1;
}
#endif
```

## network_staged_message_commit.c

```
// network_staged_message_commit  (Ghidra: FUN_004da250; renamed, no prior name)
// address 0x4da250, size 199 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Commits a previously-staged outgoing
// message to the send queue while the connection is in state 2; otherwise a no-op"). This
// function has zero callers anywhere in the binary (out/functions.json callers=0); rewritten
// anyway per the task instructions. Shares the challenge/send idiom with the rest of this
// cluster (network_session_info_packet_send.c etc).
// register convention: the client pointer arrives in ECX and a 16-bit value in AX; AX is stored
// (mov [esp+8],ax at 0x4da264) into the local whose address is the payload (EDX) of
// network_prepare_challenge_packet(0x13). Every path returns 1 (AL).
// blam-cc: ECX -> client, AX -> message_value

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from challenge; the C passed placeholders or dropped the arguments.

// VERIFIED against disassembly 0x4da250..0x4da317 (2026-09-30): FIXED: AX (message_value) is stored into the payload local passed to network_prepare_challenge_packet; send sequence compared, always returns 1
```

```
#if 0
Original Ghidra decompilation (0x4da250):

undefined4 FUN_004da250(void)

{
  int iVar1;
  char cVar2;
  ushort *puVar3;
  int in_ECX;
  int iVar4;

  if (*(short *)(in_ECX + 0xeda) != 2) {
    return 1;
  }
  puVar3 = (ushort *)network_prepare_challenge_packet();
  if (puVar3 != (ushort *)0x0) {
    iVar1 = *(int *)(in_ECX + 0xadc);
    iVar4 = (uint)(*puVar3 >> 4) * 8;
    if ((*(byte *)(iVar1 + 0xa8c) & 1) == 0) {
      if ((((*(int *)(iVar1 + 0x24) + *(int *)(iVar1 + 0x1c) * -8) - *(int *)(iVar1 + 0x20)) + 1 <
           iVar4 + 1) && (cVar2 = FUN_004ddb60(iVar1,1), cVar2 == '\0')) {
        return 1;
      }
      *(int *)(iVar1 + 0xa80) = *(int *)(iVar1 + 0xa80) + iVar4 + 1;
      bit_stream_write_bits_chunked(1);
      *(undefined1 *)(iVar1 + 0x2c) = 0;
      bit_stream_write_bits_chunked(iVar4);
      *(undefined1 *)(iVar1 + 0x2c) = 0;
    }
    return 1;
  }
  return 1;
}
#endif
```
