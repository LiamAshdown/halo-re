# Original notes: networking `net1_decode`

Author notes and decompile blocks moved verbatim from the C files converted into the net1_decode sources.

## network_game_action_apply.c

```
// network_game_action_apply  (Ghidra: FUN_004da320; renamed, no prior name)
// address 0x4da320, size 787 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/networking_functions.md summary ("Applies a single queued network-game
// action by type id, calling the specific per-type handler (ammo pickups, player/vehicle
// network updates, etc.)").
// blam-cc: EAX -> context, ECX -> client
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly and both jump tables (client 0x4da634, 0x38
// entries; host index bytes 0x4da734 / cases 0x4da714). The context is the decode context
// network_game_action_queue_drain builds (context[0] the decode state, whose +4 is the action type;
// context[0x11] the decoded record); the client arrives in ECX and is kept in ESI. Every handler takes the context
// in EAX (or EDX / on the stack where noted); the client goes to object_type_override_call_0x70_release_node
// (stack), game_engine_invoke_profile_post_update_callback (ECX) and network_channel_key_send_state (ESI). The
// "applying" flag 0x71c2c0 is set before the dispatch in both modes and cleared after (also for an unknown type).
// When network_game_mode is 1 (client) all types run; when 2 (host) only 6, 0xb, 0xf, 0x1a, 0x21, 0x22 and 0x35. The
// previous C called ~45 handlers with no arguments at all.
```

```
#if 0
Original Ghidra decompilation (0x4da320):

void FUN_004da320(void)

{
  int *in_EAX;

  if (DAT_00719720 != 1) {
    if (DAT_00719720 != 2) {
      return;
    }
    switch(*(undefined4 *)(*in_EAX + 4)) {
    case 6:
      goto switchD_004da34c_caseD_6;
    default:
LAB_004da631:
      DAT_0071c2c0 = 0;
      return;
    case 0xb:
switchD_004da34c_caseD_b:
      DAT_0071c2c0 = 1;
      FUN_00456ad0();
      DAT_0071c2c0 = 0;
      return;
    case 0xf:
switchD_004da34c_caseD_f:
      DAT_0071c2c0 = 1;
      FUN_004aaf70();
      DAT_0071c2c0 = 0;
      return;
    case 0x1a:
switchD_004da34c_caseD_1a:
      DAT_0071c2c0 = 1;
      FUN_00470a10();
      DAT_0071c2c0 = 0;
      return;
    case 0x21:
switchD_004da34c_caseD_21:
      DAT_0071c2c0 = 1;
      FUN_004de950();
      DAT_0071c2c0 = 0;
      return;
    case 0x22:
switchD_004da34c_caseD_22:
      DAT_0071c2c0 = 1;
      message_delta_parameters_protocol_receive_update();
      FUN_004ec390();
      DAT_0071c2c0 = 0;
      return;
    case 0x35:
switchD_004da34c_caseD_35:
      DAT_0071c2c0 = 1;
      FUN_004dbaa0();
      DAT_0071c2c0 = 0;
      return;
    }
  }
  DAT_0071c2c0 = 1;
  switch(*(undefined4 *)(*in_EAX + 4)) {
  case 0:
    object_delete_by_pooled_node_id();
    DAT_0071c2c0 = 0;
    return;
  case 1:
  case 2:
  case 3:
    object_type_override_call_0x70_release_node();
    DAT_0071c2c0 = 0;
    return;
  case 4:
    object_type_override_call_0x70_release_node();
    DAT_0071c2c0 = 0;
    return;
  case 5:
    object_type_override_call_0x70_release_node();
    DAT_0071c2c0 = 0;
    return;
  case 6:
switchD_004da34c_caseD_6:
    DAT_0071c2c0 = 1;
    FUN_004ae200();
    DAT_0071c2c0 = 0;
    return;
  case 7:
    FUN_004778c0();
    DAT_0071c2c0 = 0;
    return;
  case 8:
    FUN_00477c70();
    DAT_0071c2c0 = 0;
    return;
  case 9:
    FUN_0056c400();
    DAT_0071c2c0 = 0;
    return;
  case 10:
    FUN_00478f10();
    DAT_0071c2c0 = 0;
    return;
  case 0xb:
    goto switchD_004da34c_caseD_b;
  case 0xc:
    FUN_00566c90();
    DAT_0071c2c0 = 0;
    return;
  default:
    goto LAB_004da631;
  case 0xe:
    FUN_00479b40();
    DAT_0071c2c0 = 0;
    return;
  case 0xf:
    goto switchD_004da34c_caseD_f;
  case 0x10:
  case 0x11:
  case 0x12:
  case 0x13:
  case 0x14:
    FUN_00466e60();
    DAT_0071c2c0 = 0;
    return;
  case 0x15:
    FUN_00466d00();
    DAT_0071c2c0 = 0;
    return;
  case 0x16:
    FUN_00467230();
    DAT_0071c2c0 = 0;
    return;
  case 0x17:
    FUN_00468320();
    DAT_0071c2c0 = 0;
    return;
  case 0x18:
    FUN_004609d0();
    DAT_0071c2c0 = 0;
    return;
  case 0x19:
    FUN_0046bca0();
    DAT_0071c2c0 = 0;
    return;
  case 0x1a:
    goto switchD_004da34c_caseD_1a;
  case 0x1b:
    FUN_0056ddb0();
    DAT_0071c2c0 = 0;
    return;
  case 0x1c:
    FUN_00572110();
    DAT_0071c2c0 = 0;
    return;
  case 0x1d:
    FUN_0055b110();
    DAT_0071c2c0 = 0;
    return;
  case 0x1e:
    FUN_004c0ca0();
    DAT_0071c2c0 = 0;
    return;
  case 0x1f:
    FUN_004bbe20();
    DAT_0071c2c0 = 0;
    return;
  case 0x20:
    FUN_004c5c10();
    DAT_0071c2c0 = 0;
    return;
  case 0x21:
    goto switchD_004da34c_caseD_21;
  case 0x22:
    goto switchD_004da34c_caseD_22;
  case 0x23:
    player_update_client_local_player_update_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x24:
    player_update_client_local_player_vehicle_update_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x25:
    player_update_client_remote_player_action_update_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x26:
    FUN_004e5720();
    DAT_0071c2c0 = 0;
    return;
  case 0x27:
    player_update_client_remote_player_position_delta_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x28:
    player_update_client_remote_player_vehicle_position_delta_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x29:
    player_update_client_remote_player_total_biped_update_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x2a:
    player_update_client_remote_player_total_vehicle_update_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x2b:
    FUN_004c3530();
    DAT_0071c2c0 = 0;
    return;
  case 0x2c:
    item_add_ammunition();
    DAT_0071c2c0 = 0;
    return;
  case 0x2d:
    FUN_004c3870();
    DAT_0071c2c0 = 0;
    return;
  case 0x2e:
    FUN_004c4ac0();
    DAT_0071c2c0 = 0;
    return;
  case 0x2f:
    FUN_0045f8f0();
    DAT_0071c2c0 = 0;
    return;
  case 0x30:
    FUN_004bdb40();
    DAT_0071c2c0 = 0;
    return;
  case 0x31:
    object_apply_linked_impulse();
    DAT_0071c2c0 = 0;
    return;
  case 0x32:
    object_apply_shield_charge_and_notify();
    DAT_0071c2c0 = 0;
    return;
  case 0x33:
    FUN_004bf1c0();
    DAT_0071c2c0 = 0;
    return;
  case 0x35:
    goto switchD_004da34c_caseD_35;
  case 0x37:
    network_client_handle_server_text_message();
    DAT_0071c2c0 = 0;
    return;
  }
}
#endif
```

## network_game_action_queue_drain.c

```
// network_game_action_queue_drain  (Ghidra: FUN_004db870; renamed, no prior name)
// address 0x4db870, size 303 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (REWRITTEN; was 0.2 (LOW -- several unresolved data-flow gaps; see
// UNSURE notes))
// evidence: out/phase4/networking_functions.md summary ("Drains and applies a bounded run of
// queued network-game actions, one per iteration, via the action dispatcher"). Ghidra's own
// decompile carries a "Restarted to delay deadcode elimination for space: stack" warning.
// register convention: Ghidra's own recovered stack parameters (client, a bound value, the
// expected-sequence pointer). // blam-cc: stack -> client, bound, expected_sequence
// UNSURE (major): `local_98`/`local_97` (this file's `type_flag_a`/`type_flag_b`) are tested
// every loop iteration but never assigned anywhere in Ghidra's visible output except to 0 at the
// top of each iteration -- meaning, as decompiled, they can never be nonzero and the "success"
// branch of the loop body is unreachable. This is the same class of information loss already
// flagged in player_data_iterator_advance.c and network_disconnect_notify_dropped_machines.c
// (almost certainly set by message_delta_decode_array_field or network_game_action_apply through registers/an
// implicit output Ghidra did not track), transcribed literally rather than invented.
// UNSURE: `param_2` (Ghidra's second stack parameter) is never read anywhere in the visible
// body, yet `local_ac` (this file's `bound`) is compared against a running count with no visible
// initializer either. Reconstructed as `bound = param_2`, the only value in scope that matches
// the "bounded run" framing in the summary; `local_9c` (`applied_count`) is initialized to 0,
// also not shown explicitly.
// UNSURE: `network_channel_remote_address_or_default`, `message_delta_decode_array_field` and `network_game_action_apply` are
// all called with no visible arguments; reconstructed per the established conventions of this
// cluster (client + a fresh decode_result for the guard; a fresh scratch "action" record,
// mirroring network_game_action_apply's own established EAX->action_entry convention, for the
// other two).
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4db870..0x4db99f): the arguments are
// the client, the item's bit stream and the sender address (not a bound and a sequence). The channel's remote
// address (client +0xadc, 0x4dd390) must match the sender; then a 0x34-byte decode state is begun on the stream
// (message_delta_decode_begin: EAX state, EDI stream) and the decode context is built: context[0] = the state,
// context[1..16] zero, context[0x11] = a zeroed 0x80-byte action record. Each message_delta_decode_array_field
// (EAX context) that succeeds is applied (network_game_action_apply: EAX context, ECX client); the state's +0x18
// counts them; the run continues only while both state flags +0x1c/+0x1d came back 1, and ends with 1 once the
// count passes the state's +0x08. Any other end notifies dropped machines (EBX client) and returns 0.
```

```
#if 0
Original Ghidra decompilation (0x4db870):

/* WARNING: Restarted to delay deadcode elimination for space: stack */

char FUN_004db870(undefined4 param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  char local_119;
  undefined4 local_114 [16];
  undefined1 *local_d4;
  int local_cc;
  int local_ac;
  int local_9c;
  char local_98;
  char local_97;
  undefined1 local_80;
  undefined4 local_7f;

  FUN_004dd390();
  if ((local_cc == *param_3) && (cVar1 = FUN_004ec490(), cVar1 != '\0')) {
    local_80 = 0;
    puVar3 = &local_7f;
    for (iVar2 = 0x1f; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    *(undefined2 *)puVar3 = 0;
    *(undefined1 *)((int)puVar3 + 2) = 0;
    puVar3 = local_114;
    for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    local_d4 = &local_80;
    while( true ) {
      local_97 = '\0';
      local_98 = '\0';
      cVar1 = FUN_004ec510();
      if (cVar1 == '\0') {
        local_119 = '\0';
        goto LAB_004db984;
      }
      FUN_004da320();
      if ((local_98 == '\x01') && (local_97 == '\x01')) {
        local_119 = '\x01';
      }
      else {
        local_119 = '\0';
      }
      local_9c = local_9c + 1;
      puVar3 = local_114;
      for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar3 = 0;
        puVar3 = puVar3 + 1;
      }
      local_98 = '\0';
      local_97 = '\0';
      if (local_119 != '\x01') break;
      if (local_ac < local_9c) {
        return '\x01';
      }
    }
    if (local_119 != '\0') {
      return local_119;
    }
  }
  else {
    local_119 = '\0';
  }
LAB_004db984:
  FUN_004d9340();
  return local_119;
}
#endif
```

## network_game_client_decode_and_discard_ingame_message.c

```
// network_game_client_decode_and_discard_ingame_message  (Ghidra: FUN_004dc020; renamed, no
// prior name)
// address 0x4dc020, size 99 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
// evidence: out/phase4/networking_functions.md summary ("Decodes and discards an in-game
// message type (0xd) using a different message-group index than the join-handshake handlers").
// Structurally identical to network_game_client_decode_and_discard_join_message.c except for
// the mode check (4, in-game, instead of 2) and the message-class argument (6 instead of 2).
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
```

```
#if 0
Original Ghidra decompilation (0x4dc020):

undefined4 FUN_004dc020(int param_1,undefined4 param_2,int *param_3)

{
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 4)) {
    data_packet_group_decode_packet
              (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,&param_3,6)
    ;
  }
  return 1;
}
#endif
```

## network_game_client_decode_and_discard_join_message.c

```
// network_game_client_decode_and_discard_join_message  (Ghidra: FUN_004dbfb0; renamed, no prior
// name)
// address 0x4dbfb0, size 101 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
// evidence: out/phase4/networking_functions.md summary ("Decodes and discards a message type
// (0xc) received during the join handshake, taking no further action").
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
```

```
#if 0
Original Ghidra decompilation (0x4dbfb0):

undefined4 FUN_004dbfb0(int param_1,undefined4 param_2,int *param_3)

{
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 2)) {
    data_packet_group_decode_packet
              (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,&param_3,2)
    ;
  }
  return 1;
}
#endif
```

## network_game_client_decode_beacon_reply.c

```
// network_game_client_decode_beacon_reply  (Ghidra: network_game_client_decode_beacon_reply,
// already named)
// address 0x4db9a0, size 119 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (step 1: rewritten from objdump -d 0x4db9a0..0x4dba0f)
// evidence: out/phase4/networking_functions.md summary ("Decodes an incoming game-announcement
// ('beacon') message received while idle and adds it to the discovered-games search list").
// register convention: client in EAX (in_EAX), the incoming buffer in EDX (in_EDX).
// blam-cc: EAX -> client, EDX -> buffer
// UNSURE (major): `network_game_search_results_add_or_update` (0x4da7d0, this task's batch) is
// called with a single stack argument, `client + 4` -- but that function's own decompiled body
// (a genuine stack parameter plus a register `announcement`) takes the results-table pointer on
// the stack and the announcement in a register. That means client+4 is the results-table
// argument, which is new evidence that types/networking.h's undeclared 9-slot
// network_game_search_entry table lives at client+4 (inside the still-unresolved
// unknown_002[0xada] block) -- worth a header update in a future pass. The announcement
// argument itself is not visibly set at this call site; reconstructed as `buffer`, the only
// address in scope that could be the decoded announcement payload.
// UNSURE: `data_packet_group_decode_packet` is called here with only 6 of its fuller,
// separately-reconstructed 8-argument signature (see
// network_game_client_decode_state_update_chunk.c); called here with a matching 6-argument local
// prototype instead of forcing the mismatch, per the same reasoning documented in
// network_send_join_request_packet.c for data_packet_group_encode_packet.
```

```
#if 0
Original Ghidra decompilation (0x4db9a0):

undefined4 network_game_client_decode_beacon_reply(void)

{
  char cVar1;
  int in_EAX;
  int in_EDX;
  undefined1 local_178 [4];
  undefined1 local_174 [4];
  undefined1 local_170 [368];

  if (*(short *)(in_EAX + 0xeda) == 0) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_170,in_EDX + 2,local_178,
                       local_174,1);
    if (cVar1 != '\0') {
      network_game_search_results_add_or_update(in_EAX + 4);
    }
    return 1;
  }
  return 1;
}
#endif
```

## network_game_client_decode_connect_rejected.c

```
// network_game_client_decode_connect_rejected  (Ghidra: FUN_004dbd40; renamed, no prior name)
// address 0x4dbd40, size 124 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Decodes a connect-rejected / error
// notification from the server and triggers the client disconnect path with the given error
// code"). Same guard/decode shape as the rest of this handler cluster.
// register convention: client in EAX (in_EAX); stack -> buffer, capacity, expected_sequence.
// blam-cc: EAX -> client; stack -> buffer, capacity, expected_sequence
// UNSURE: `network_session_disconnect_with_error` (network_session_disconnect_with_error, this batch) is called here with
// no visible argument; reconstructed as taking the decoded record's first word (the only
// plausible source of "the given error code" per the summary).
// UNSURE: the real return value is `local_18 & 0xffffff00` in Ghidra (always 0, since local_18
// only ever holds a decode-success byte or the disconnect call's own boolean result by this
// point); modeled directly as 0.

// FIXED 2026-09-28 (networking call audit, from the disassembly): the third argument is the record length (an int);
// the original reduces it by 2 in its own slot and passes its address (EAX) as data_packet_group_decode_packet's
// remaining length -- that argument was missing from the declaration, so every other argument was shifted by one.
```

```
#if 0
Original Ghidra decompilation (0x4dbd40):

uint FUN_004dbd40(int param_1,undefined4 param_2,uint *param_3)

{
  int in_EAX;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  uint local_18;

  FUN_004dd390();
  if (((local_18 == *param_3) && (*(short *)(in_EAX + 0xeda) != 0)) &&
     (*(short *)(in_EAX + 0xeda) != 4)) {
    local_18 = data_packet_group_decode_packet
                         (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,
                          &param_3,2);
    if ((char)local_18 != '\0') {
      local_18 = FUN_004d97e0();
    }
  }
  return local_18 & 0xffffff00;
}
#endif
```

## network_game_client_decode_join_accepted.c

```
// network_game_client_decode_join_accepted  (Ghidra: network_game_client_decode_join_accepted,
// already named)
// address 0x4dbcc0, size 127 bytes
// name confidence: 0.55   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
// evidence: out/phase4/networking_functions.md summary ("Decodes the server's join-accepted
// message, assigning the local client its player slot index and advancing the connection to the
// next handshake state"). Same guard/decode shape as network_game_decode_settings_request.c.
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
// UNSURE: `data_packet_group_decode_packet`'s 5th argument is literally `&param_3` in Ghidra
// (the address of the caller's own `expected_sequence` pointer parameter, i.e. a
// pointer-to-pointer) rather than a fresh local buffer -- preserved exactly, though it looks
// like it could be a decompiler artifact rather than intentional.
// UNSURE: `network_channel_remote_address_or_default` and `network_session_player_join_notify` are both
// called with no visible arguments beyond what Ghidra shows; reconstructed per the established
// conventions of this cluster.
```

```
#if 0
Original Ghidra decompilation (0x4dbcc0):

undefined4 network_game_client_decode_join_accepted(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  undefined1 local_24 [4];
  undefined1 local_20 [8];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 1)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,param_1 + 2,local_24,
                       &param_3,2);
    if (cVar1 != '\0') {
      FUN_004d9700();
      return 1;
    }
  }
  return 0;
}
#endif
```

## network_game_client_decode_join_complete.c

```
// network_game_client_decode_join_complete  (Ghidra: network_game_client_decode_join_complete,
// already named)
// address 0x4dbdc0, size 141 bytes
// name confidence: 0.55   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
// evidence: out/phase4/networking_functions.md summary ("Decodes the server's final
// join-complete confirmation and transitions the client to the in-game connection state").
// Same guard/decode shape as the rest of this handler cluster; client->state = 4 on success
// matches this cluster's other functions treating state as a live connection-mode value.
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
```

```
#if 0
Original Ghidra decompilation (0x4dbdc0):

undefined4 network_game_client_decode_join_complete(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if (local_18 == *param_3) {
    if ((*(short *)(unaff_ESI + 0xeda) != 0) && (*(short *)(unaff_ESI + 0xeda) != 4)) {
      cVar1 = data_packet_group_decode_packet
                        (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,
                         &param_3,2);
      if (cVar1 != '\0') {
        DAT_00718f8c = 9;
        *(undefined2 *)(unaff_ESI + 0xeda) = 4;
        return 1;
      }
    }
  }
  return 0;
}
#endif
```

## network_game_client_decode_join_finalize_ack.c

```
// network_game_client_decode_join_finalize_ack  (Ghidra: FUN_004dc120; renamed, no prior name)
// address 0x4dc120, size 112 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
// evidence: out/phase4/networking_functions.md summary ("Decodes a join-handshake message (type
// 0xb) and forwards the payload to a follow-up handler"). Forwards to
// network_client_timer_default_or_disconnect.c (network_client_timer_default_or_disconnect, 0x4d9ce0, this task's batch).
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
// UNSURE: `network_client_timer_default_or_disconnect` is called here with no visible argument;
// reconstructed as taking `client`, matching that function's own established signature.
```

```
#if 0
Original Ghidra decompilation (0x4dc120):

undefined4 FUN_004dc120(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 2)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,
                       &param_3,2);
    if (cVar1 != '\0') {
      FUN_004d9ce0();
    }
  }
  return 1;
}
#endif
```

## network_game_client_decode_join_finalize_message.c

```
// network_game_client_decode_join_finalize_message  (Ghidra: FUN_004dc090; renamed, no prior
// name)
// address 0x4dc090, size 130 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
// evidence: out/phase4/networking_functions.md summary ("Decodes a join-handshake message (type
// 0xa) and forwards it to a secondary validation/processing routine, propagating its
// success/failure result"); the "secondary routine" is network_connection_finalize_join.c
// (0x4d9960, this task's batch), called with the client pointer.
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
```

```
#if 0
Original Ghidra decompilation (0x4dc090):

int FUN_004dc090(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int iVar2;
  ushort *unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if (local_18 != *param_3) {
    return 1;
  }
  if (unaff_ESI[0x76d] == 2) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,
                       &param_3,2);
    if (cVar1 != '\0') {
      iVar2 = network_connection_finalize_join(unaff_ESI);
      return iVar2;
    }
  }
  return 0;
}
#endif
```

## network_game_client_decode_player_config_value.c

```
// network_game_client_decode_player_config_value  (Ghidra: FUN_004dbf30; renamed, no prior name)
// address 0x4dbf30, size 117 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
// evidence: out/phase4/networking_functions.md summary ("Decodes a small per-player
// configuration value from the server and stores it directly into the connection context during
// the join handshake"). client+0xed8 matches types/networking.h's
// network_client_globals::unknown_ed8 exactly.
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
// UNSURE: `data_packet_group_decode_packet`'s 2nd argument (the decode destination) is literally
// `&param_3` in Ghidra -- the address of the caller's own `expected_sequence` pointer parameter
// -- and the decoded value is then read back from that same location. Preserved exactly, though
// it looks like it could be a decompiler artifact rather than intentional; modeled here by
// decoding into a local and writing that local's address into `*expected_sequence_slot`, which
// reproduces the same "decode result ends up reachable through the expected_sequence parameter"
// shape.
```

```
#if 0
Original Ghidra decompilation (0x4dbf30):

undefined4 FUN_004dbf30(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 2)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,&param_3,param_1 + 2,local_1c,
                       local_20,2);
    if (cVar1 != '\0') {
      *(undefined2 *)(unaff_ESI + 0xed8) = param_3._0_2_;
    }
  }
  return 1;
}
#endif
```

## network_game_client_decode_player_join_chunk.c

```
// network_game_client_decode_player_join_chunk  (Ghidra: FUN_004dc240; named per this rewrite)
// address 0x4dc240, size 156 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Decodes another synchronization-phase data
// chunk during the join handshake, aborting via the shared cleanup routine on failure." On
// success it calls network_player_join_finalize (0x4d9e30, already named, "Creates the
// game-object datum for a player once their connection has fully joined"), which gives this
// handler its name. Accepted only while client->state == 3; when it is 4 the message is
// silently treated as consumed with no decode attempt.
// register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
// general EAX->client, param_2->remaining_length reconstruction this whole cluster shares, and
// network_channel_remote_address_or_default.c for the local decode-result record it fills.
// UNSURE: network_player_join_finalize's own decompile (0x4d9e30) shows it taking an elided
// EAX (a network_player_entry *) and EDI (client, viewed as ushort * for word-indexed access at
// +0x58a == byte +0xb14 and +0x76d == byte +0xeda, both matching this cluster's client offsets)
// with no arguments visible at ITS call sites either -- the same Ghidra-drops-dead-looking-loads
// problem as network_channel_remote_address_or_default. Passing just `client` here is a lower bound on what
// the original call needed; the player-entry argument could not be reconstructed from this
// function's own decompilation and is left out, which is the single largest risk to this file's
// rewrite confidence.

// FIXED 2026-09-28 (networking call audit, from the disassembly): the dispatcher (0x4db6b0) passes (client, record,
// length, sender) -- the length is an int, which the original reduces by 2 in its own argument slot and hands to
// data_packet_group_decode_packet by address (EAX) as the remaining length; that call takes 7 arguments (remaining,
// group, body, record + 2, out_type, out_version_used, expected class), not 8 (the extra one made the class 0);
// network_disconnect_notify_dropped_machines gets the client (EBX).
```

```
#if 0
Original Ghidra decompilation (0x4dc240):

char FUN_004dc240(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int in_EAX;
  char local_25;
  undefined1 local_24 [4];
  int local_20 [8];

  local_25 = '\0';
  FUN_004dd390();
  if (local_20[0] != *param_3) {
    return '\x01';
  }
  if (*(short *)(in_EAX + 0xeda) == 3) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,param_1 + 2,local_24,
                       &param_3,4);
    if ((cVar1 != '\0') && (local_25 = network_player_join_finalize(), local_25 != '\0')) {
      return local_25;
    }
  }
  else if (*(short *)(in_EAX + 0xeda) == 4) {
    return '\x01';
  }
  FUN_004d9340();
  return local_25;
}
#endif
```

## network_game_client_decode_player_slot_chunk.c

```
// network_game_client_decode_player_slot_chunk  (Ghidra: FUN_004dc2e0; named per this rewrite)
// address 0x4dc2e0, size 183 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Decodes a third variant of synchronization-phase
// data chunk, accepted across a wider range of connection states." On success it calls
// network_session_player_table_index_apply, whose own summary ("Finds the player slot matching a given machine id and
// records its assigned table index (e.g. team or score-table slot) for that player") gives this
// handler its name. Accepted while client->state is 2, 3 or 4.
// register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
// shared EAX/ESI->client, param_2->remaining_length reconstruction, and
// network_channel_remote_address_or_default.c for the local decode-result record it fills. Here the
// client pointer arrives via ESI (unaff_ESI) rather than EAX, still the sole blam-cc argument.
// The trailing "else if (state == 4) return 1" is unreachable in the original -- state == 4 is
// already covered by the first branch's condition -- and is kept verbatim rather than removed,
// since preserving control flow exactly takes priority over simplifying apparently-dead code
// Ghidra may be reporting faithfully from the real binary.

// FIXED 2026-09-28 (networking call audit, from the disassembly): the dispatcher (0x4db6b0) passes (client, record,
// length, sender) -- the length is an int, which the original reduces by 2 in its own argument slot and hands to
// data_packet_group_decode_packet by address (EAX) as the remaining length; that call takes 7 arguments (remaining,
// group, body, record + 2, out_type, out_version_used, expected class), not 8 (the extra one made the class 0);
// network_disconnect_notify_dropped_machines gets the client (EBX).

// VERIFIED against disassembly 0x4dc2e0..0x4dc397 (2026-09-30): sender check, state 2/3/4 gate (trailing state==4 test is unreachable in the binary too), decode args (remaining in EAX, class 4), apply(client, body+0x20, EBX body), notify_dropped(EBX client)
```

```
#if 0
Original Ghidra decompilation (0x4dc2e0):

char FUN_004dc2e0(int param_1,undefined4 param_2,int *param_3)

{
  short sVar1;
  char cVar2;
  int unaff_ESI;
  char local_29;
  undefined1 local_28 [4];
  int local_24 [9];

  local_29 = '\0';
  FUN_004dd390();
  if (local_24[0] != *param_3) {
    return '\x01';
  }
  sVar1 = *(short *)(unaff_ESI + 0xeda);
  if (((sVar1 == 3) || (sVar1 == 4)) || (sVar1 == 2)) {
    cVar2 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_24,param_1 + 2,local_28,
                       &param_3,4);
    if ((cVar2 != '\0') && (local_29 = FUN_004d9190(), local_29 != '\0')) {
      return local_29;
    }
  }
  else if (sVar1 == 4) {
    return '\x01';
  }
  FUN_004d9340();
  return local_29;
}
#endif
```

## network_game_client_decode_pong_reply.c

```
// network_game_client_decode_pong_reply  (Ghidra: network_game_client_decode_pong_reply, already
// named)
// address 0x4dba20, size 114 bytes
// name confidence: 0.55   rewrite confidence: 0.9 (step 1: rewritten from objdump -d 0x4dba20..0x4dba91)
// evidence: out/phase4/networking_functions.md summary ("Decodes an incoming ping-reply (pong)
// message and updates the connection's round-trip-time/ping statistics").
// register convention: client in EAX (in_EAX), the incoming buffer in EDX (in_EDX).
// blam-cc: EAX -> client, EDX -> buffer
// UNSURE: `network_connection_retransmit_if_overdue` (network_connection_retransmit_if_overdue.c, this batch) is called with
// a single argument here, `local_4` (an uninitialized 4-byte decode-output local); that
// function's own signature takes three parameters (sender_address, client, deadline_ms) in
// ECX/ESI/EDI. Reconstructed as passing `&decoded_body`-derived data is not evidenced either;
// left calling that function's first parameter slot with the decoded record pointer and 0 for
// the rest, flagged as the weakest link in this file.
// UNSURE: `data_packet_group_decode_packet`'s argument count mismatch is the same as
// network_game_client_decode_beacon_reply.c; see that file's header note.
```

```
#if 0
Original Ghidra decompilation (0x4dba20):

undefined4 network_game_client_decode_pong_reply(void)

{
  char cVar1;
  int in_EAX;
  int in_EDX;
  undefined1 local_10 [4];
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined4 local_4;

  if ((*(short *)(in_EAX + 0xeda) != 0) && (*(short *)(in_EAX + 0xeda) != 3)) {
    return 1;
  }
  cVar1 = data_packet_group_decode_packet
                    (&PTR_s_network_game_messages_group_006994f8,local_8,in_EDX + 2,local_c,local_10
                     ,1);
  if (cVar1 != '\0') {
    FUN_004d93b0(local_4);
  }
  return 1;
}
#endif
```

## network_game_client_decode_settings_or_ack.c

```
// network_game_client_decode_settings_or_ack  (Ghidra: FUN_004dbe50; renamed, no prior name)
// address 0x4dbe50, size 218 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Decodes an incoming game-settings/
// map-info message during the join handshake and applies it, sending a one-time acknowledgement
// when acting purely as a client"). Forwards to network_game_settings_packet_receive.c
// (0x4d9800) and network_game_settings_ack_send.c (0x4d9f50), both this task's batch.
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
// UNSURE: `network_game_settings_ack_send` is called here with no visible arguments;
// reconstructed as (client, 0) matching network_game_settings_packet_receive.c's own
// reconstruction of the same call.

// FIXED 2026-09-28 (networking call audit, from the disassembly): the third argument is the record length (an int);
// the original reduces it by 2 in its own slot and passes its address (EAX) as data_packet_group_decode_packet's
// remaining length -- that argument was missing from the declaration, so every other argument was shifted by one.
```

```
#if 0
Original Ghidra decompilation (0x4dbe50):

undefined4 FUN_004dbe50(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  undefined4 uVar2;
  int unaff_ESI;
  undefined1 local_3d0 [4];
  undefined1 local_3cc [4];
  int local_3c8;
  undefined1 local_3b0 [944];

  FUN_004dd390();
  if (local_3c8 == *param_3) {
    if (DAT_00719720 == 2) {
      if ((*(short *)(unaff_ESI + 0xeda) == 2) && (*(char *)(unaff_ESI + 0xee2) == '\0')) {
        FUN_004d9f50();
        *(undefined1 *)(unaff_ESI + 0xee2) = 1;
      }
    }
    else if ((*(short *)(unaff_ESI + 0xeda) == 2) || (*(short *)(unaff_ESI + 0xeda) == 3)) {
      cVar1 = data_packet_group_decode_packet
                        (&PTR_s_network_game_messages_group_006994f8,local_3b0,param_1 + 2,local_3d0
                         ,local_3cc,2);
      if (cVar1 != '\0') {
        uVar2 = FUN_004d9800(local_3b0);
        return uVar2;
      }
      return 0;
    }
  }
  return 1;
}
#endif
```

## network_game_client_decode_state_update_chunk.c

```
// network_game_client_decode_state_update_chunk  (Ghidra: FUN_004dc190; named per this rewrite)
// address 0x4dc190, size 172 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Decodes one of several similarly-structured
// synchronization-phase data chunks and aborts the connection attempt via a shared cleanup
// routine on failure." On success it forwards to network_game_state_update_receive, whose own summary
// ("Processes an incoming sequenced game-state update packet, growing the per-connection
// reassembly buffer as needed and handing the payload off for application") gives this handler
// its name. Only accepts the message while client->state == 3.
// register/parameter convention (UNSURE, reconstructed -- see network_channel_remote_address_or_default.c's
// header for why Ghidra shows the shared guard call with no visible arguments):
//   blam-cc: EAX -> client (network_client_globals *, the global `network_client`)
//   stack: param_1 -> buffer (message bytes; the 2-byte packet header is skipped by the call to
//          data_packet_group_decode_packet below), param_2 -> UNSURE: reused as the
//          `int16_t *remaining_length` register argument data_packet_group_decode_packet needs
//          in EAX and which is otherwise dead in this function's own body, param_3 ->
//          expected_sequence (compared against the guard's local decode-result record).
// data_packet_group_decode_packet is called here with only 6 of its 7 stack arguments visible
// (see its own file's header on the general EAX-hidden-argument problem); the 7th,
// out_version_used, is supplied as a literal 0/NULL, matching the pattern data_packet_group
// itself uses at its own elided call site; the "input" byte_stream is a throwaway local scratch
// for the same reason (its address is what Ghidra shows here, but with no prior write to it).
// UNSURE: client->state is declared as padding in types/networking.h, but this whole cluster of
// functions (0x4dc190..0x4dc4b0) reads/writes it as a live join-handshake state discriminant
// (values observed: 2, 3, 4); the header's field name could not be changed here (do not edit
// types/*.h), so it is used as declared with this note.

// FIXED 2026-09-28 (networking call audit, from the disassembly): the dispatcher (0x4db6b0) passes (client, record,
// length, sender) -- the length is an int, which the original reduces by 2 in its own argument slot and hands to
// data_packet_group_decode_packet by address (EAX) as the remaining length; that call takes 7 arguments (remaining,
// group, body, record + 2, out_type, out_version_used, expected class), not 8 (the extra one made the class 0);
// network_disconnect_notify_dropped_machines gets the client (EBX).
```

```
#if 0
Original Ghidra decompilation (0x4dc190):

char FUN_004dc190(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int in_EAX;
  char local_231;
  undefined1 local_230 [4];
  undefined1 local_22c [4];
  int local_228;
  undefined1 local_210 [528];

  local_231 = '\0';
  FUN_004dd390();
  if ((local_228 != *param_3) || (*(short *)(in_EAX + 0xeda) != 3)) {
    return '\x01';
  }
  cVar1 = data_packet_group_decode_packet
                    (&PTR_s_network_game_messages_group_006994f8,local_210,param_1 + 2,local_230,
                     local_22c,4);
  if ((cVar1 != '\0') && (local_231 = FUN_004d9d20(), local_231 != '\0')) {
    return local_231;
  }
  FUN_004d9340();
  return local_231;
}
#endif
```

## network_game_client_decode_sync_complete.c

```
// network_game_client_decode_sync_complete  (Ghidra: FUN_004dc3a0; named per this rewrite)
// address 0x4dc3a0, size 108 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
// evidence: out/phase4/networking_functions.md: "Decodes the final synchronization message of the
// join handshake and transitions the connection into the fully-connected/in-game state." Matches
// the code exactly: on a matching sequence and client->state == 3, it decodes the (unused)
// payload and forces state to 4, always reporting the message consumed (1).
// register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
// shared EAX/ESI->client, param_2->remaining_length reconstruction, and
// network_channel_remote_address_or_default.c for the local decode-result record it fills.
```

```
#if 0
Original Ghidra decompilation (0x4dc3a0):

undefined4 FUN_004dc3a0(int param_1,undefined4 param_2,int *param_3)

{
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 3)) {
    data_packet_group_decode_packet
              (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,&param_3,4)
    ;
    *(undefined2 *)(unaff_ESI + 0xeda) = 4;
  }
  return 1;
}
#endif
```

## network_game_decode_settings_request.c

```
// network_game_decode_settings_request  (Ghidra: FUN_004dbc00; renamed, no prior name)
// address 0x4dbc00, size 177 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Decodes an early-handshake server/map
// identification message and compares it against locally cached data, advancing the loading UI
// state on a mismatch"); forwards the decoded record straight into
// network_game_settings_packet_send.c (0x4d94c0, same task batch), which builds and sends the
// full settings/map-data reply -- consistent with this being the *host's* handler for a
// newly-joined client's settings request. Follows the same guard/decode shape as
// network_game_client_decode_state_update_chunk.c (0x4dc190, same address family).
// register convention: client in ESI (unaff_ESI); param_1/param_2/param_3 are ordinary stack
// parameters per Ghidra's own recovery. // blam-cc: ESI -> client; stack -> buffer, capacity,
// expected_sequence
// UNSURE: `network_channel_remote_address_or_default` and `network_game_settings_packet_send` are both called
// with no visible arguments beyond what Ghidra shows; reconstructed per the established
// conventions of this cluster (client + a fresh decode_result for the guard; client + the
// decoded record for the settings-send call).
// UNSURE: `DAT_0069b350` is not declared in types/networking.h; named generically from its
// boolean-cast source.

// FIXED 2026-09-28 (networking call audit, from the disassembly): the third argument is the record length (an int);
// the original reduces it by 2 in its own slot and passes its address (EAX) as data_packet_group_decode_packet's
// remaining length -- that argument was missing from the declaration, so every other argument was shifted by one.
```

```
#if 0
Original Ghidra decompilation (0x4dbc00):

undefined4 FUN_004dbc00(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  undefined1 local_b4 [4];
  undefined1 local_b0 [4];
  int local_ac;
  undefined1 local_94 [8];
  char local_8c;

  FUN_004dd390();
  if ((local_ac == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 1)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_94,param_1 + 2,local_b4,
                       local_b0,2);
    if (cVar1 != '\0') {
      DAT_0069b350 = (uint)(local_8c == '\x01');
      FUN_004d94c0(local_94);
      return 1;
    }
  }
  return 0;
}
#endif
```

## network_game_message_decode_dispatch.c

```
// network_game_message_decode_dispatch  (Ghidra: network_game_message_decode_dispatch, already
// named)
// address 0x4db6b0, size 336 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md summary ("The central switch that dispatches a
// decoded incoming network-game message to the correct per-type handler based on a type byte in
// the packet").
// register convention: the decoded record header arrives in EDX (in_EDX), a length/offset value
// in EDI (unaff_EDI). // blam-cc: EDX -> record, EDI -> record_length
// UNSURE (major): every one of the 18 per-type handlers is called with literally zero visible
// arguments in Ghidra's output, yet each one's own rewrite (see their individual files, all in
// this task's batch except the last five) needed real parameters (client, buffer, capacity,
// expected_sequence) reconstructed from ITS OWN body. None of those reconstructed signatures can
// be satisfied from what this dispatcher alone has in scope (`record`, `record_length`). Rather
// than force an incorrect strongly-typed call at every case, every handler is called here through
// a raw zero-argument function-pointer cast, which matches Ghidra's own literal `CALL` with
// whatever registers are already live -- the most honest representation of what this specific
// function's machine code actually does, at the cost of not type-checking the handlers'
// individually-reconstructed parameter lists against each other.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4db6b0..0x4db800, jump tables
// 0x4db84c / 0x4db800): the client arrives in EAX, the record in EDX, its length in EDI and the sender address on
// the stack. A record whose first word has low bits 0 and bits 2-3 == 3 is dispatched on its last byte; every
// handler gets (client, record, length, sender) -- the binary passes the client in EAX only to those that read it,
// and the record in EDX to the first two -- and its result is returned (1 for anything else). The previous C
// called all eighteen handlers with no arguments.
```

```
#if 0
Original Ghidra decompilation (0x4db6b0):

undefined4 network_game_message_decode_dispatch(void)

{
  undefined4 uVar1;
  byte bVar2;
  ushort *in_EDX;
  int unaff_EDI;

  uVar1 = 1;
  if ((((*in_EDX & 3) == 0) && (bVar2 = (byte)*in_EDX >> 2 & 3, bVar2 != 1)) && (bVar2 == 3)) {
    switch(*(undefined1 *)((int)in_EDX + unaff_EDI + -1)) {
    case 2:
      uVar1 = network_game_client_decode_beacon_reply();
      return uVar1;
    case 3:
      uVar1 = network_game_client_decode_pong_reply();
      return uVar1;
    case 4:
      uVar1 = FUN_004dbc00();
      return uVar1;
    case 5:
      uVar1 = network_game_client_decode_join_accepted();
      return uVar1;
    case 6:
      uVar1 = FUN_004dbd40();
      return uVar1;
    case 7:
      uVar1 = network_game_client_decode_join_complete();
      return uVar1;
    case 8:
      uVar1 = FUN_004dbe50();
      return uVar1;
    case 9:
      uVar1 = FUN_004dbf30();
      return uVar1;
    case 10:
      uVar1 = FUN_004dc090();
      return uVar1;
    case 0xb:
      uVar1 = FUN_004dc120();
      return uVar1;
    case 0xc:
      uVar1 = FUN_004dbfb0();
      return uVar1;
    case 0xd:
      uVar1 = FUN_004dc020();
      return uVar1;
    case 0x16:
      uVar1 = FUN_004dc190();
      return uVar1;
    case 0x17:
      uVar1 = FUN_004dc240();
      return uVar1;
    case 0x18:
      uVar1 = FUN_004dc2e0();
      return uVar1;
    case 0x19:
      uVar1 = FUN_004dc3a0();
      return uVar1;
    case 0x21:
      uVar1 = FUN_004dc410();
      return uVar1;
    case 0x22:
      uVar1 = FUN_004dc4b0();
    }
  }
  return uVar1;
}
#endif
```

## network_game_message_decode_ingame_notification.c

```
// network_game_message_decode_ingame_notification  (Ghidra: FUN_004dc4b0; named per this rewrite)
// address 0x4dc4b0, size 166 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
// evidence: out/phase4/networking_functions.md: "Decodes an in-game notification and, unless a
// particular game-engine state flag is already set, triggers the client disconnect/leave path --
// consistent with a game-over or host-shutdown notice." Matches the code: once decoded (packet
// class 6, only while client->state == 4), it defaults client->unknown_edc to 8 if still zero,
// then leaves the game (network_host_handoff_requested = 1, chat_close()) unless there is a
// network_server AND its flags bit 2 is set.
// register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
// shared ESI->client, param_2->remaining_length reconstruction, and
// network_channel_remote_address_or_default.c for the local decode-result record it fills.
// UNSURE: network_server_globals.flags documents bit2 as "stats logging" in types/networking.h,
// which would make "skip disconnect while stats logging is on" an odd rule; this rewrite keeps
// the raw bit test against the header's declared field rather than asserting a different meaning.
```

```
#if 0
Original Ghidra decompilation (0x4dc4b0):

bool FUN_004dc4b0(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  bool bVar2;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  bVar2 = false;
  FUN_004dd390();
  if (local_18 == *param_3) {
    if (*(short *)(unaff_ESI + 0xeda) == 4) {
      cVar1 = data_packet_group_decode_packet
                        (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,
                         &param_3,6);
      bVar2 = cVar1 != '\0';
      if (*(short *)(unaff_ESI + 0xedc) == 0) {
        *(undefined2 *)(unaff_ESI + 0xedc) = 8;
      }
      if ((DAT_0071c2d4 == 0) || ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) == 0)) {
        DAT_0071c2de = 1;
        chat_close();
      }
    }
    return bVar2;
  }
  return true;
}
#endif
```

## network_game_message_decode_replicated_command.c

```
// network_game_message_decode_replicated_command  (Ghidra: FUN_004dc410; named per this rewrite)
// address 0x4dc410, size 147 bytes
// name confidence: 0.25   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Decodes an in-game message and, only when acting
// as host, forwards its two payload values to a follow-up handler -- consistent with the host
// applying a command replicated by a client." That description does not match the code as
// decoded: the follow-up call only runs when network_game_mode == 1, and types/networking.h
// documents mode 1 as "client", not host (0 local, 1 client, 2 host, 3 replay). Kept neutral in
// this rewrite's name pending resolution; the exact code below is preserved either way. On
// success it decodes an 8-byte payload (packet class 6, not 4 like this cluster's other
// handlers) and forwards its two dwords to network_client_timer_schedule ("Schedules a delayed network
// event/timer to fire after a given number of milliseconds").
// register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
// shared EAX->client, param_2->remaining_length reconstruction, and
// network_channel_remote_address_or_default.c for the local decode-result record it fills.
// UNSURE: network_client_timer_schedule's own decompile (0x4d9ed0) shows an elided unaff_ESI it uses for
// client->unknown_ee4-region timer fields (see that function's own offsets, +0xee8/+0xef0/+0xee4/
// +0xeec/+0xef4, all inside types/networking.h's network_client_globals.unknown_ee4[11]); this
// call site shows only the two stack arguments Ghidra's own network_client_timer_schedule signature declares, so
// `client` is supplied for the elided ESI slot by the same reasoning applied throughout this
// batch.

// FIXED 2026-09-28 (networking call audit, from the disassembly): the dispatcher (0x4db6b0) passes (client, record,
// length, sender) -- the length is an int, which the original reduces by 2 in its own argument slot and hands to
// data_packet_group_decode_packet by address (EAX) as the remaining length; that call takes 7 arguments (remaining,
// group, body, record + 2, out_type, out_version_used, expected class), not 8 (the extra one made the class 0);
// network_disconnect_notify_dropped_machines gets the client (EBX).
```

```
#if 0
Original Ghidra decompilation (0x4dc410):

undefined4 FUN_004dc410(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int in_EAX;
  undefined1 local_24 [4];
  undefined4 local_20;
  undefined4 local_1c;
  int local_18;

  FUN_004dd390();
  if (local_18 != *param_3) {
    return 1;
  }
  if ((*(short *)(in_EAX + 0xeda) == 4) &&
     (cVar1 = data_packet_group_decode_packet
                        (&PTR_s_network_game_messages_group_006994f8,&local_20,param_1 + 2,local_24,
                         &param_3,6), cVar1 != '\0')) {
    if (DAT_00719720 != 1) {
      return 1;
    }
    FUN_004d9ed0(local_20,local_1c);
    return 1;
  }
  return 0;
}
#endif
```

## network_game_process_incoming_messages.c

```
// network_game_process_incoming_messages  (Ghidra: network_game_process_incoming_messages,
// already named)
// address 0x4db180, size 388 bytes
// name confidence: 0.6   rewrite confidence: 0.85 (REWRITTEN; was 0.4)
// evidence: out/phase4/networking_types_notes.md/header comment: "Drains the channel's
// incoming ring buffer, extracting queued items bit-by-bit and dispatching each to the
// network-message/action processor." client->channel->incoming matches
// types/networking.h's network_channel::incoming (a types/memory.h circular_buffer).
// register convention: __cdecl, single stack parameter `client`.
// // blam-cc: stack -> client
// UNSURE: the inner bit-walk (`local_c`/`local_10`/`local_8`/etc) is transcribed as literally as
// possible from Ghidra's own local variables rather than renamed into a clean bit-cursor
// abstraction, since its exact edge-case behaviour (the `uVar7 == local_8 + 1` boundary check)
// is delicate and not independently re-derivable with confidence.
// UNSURE: `DAT_00861de0` (the scratch buffer network_channel_incoming_read_item fills) has no declared size in
// types/networking.h; declared here as a byte array sized from k_network_channel_stream_bits/8
// (0x2880 bits = 0x510 bytes), the module's own documented per-direction stream capacity.
// UNSURE: `item` (Ghidra's `local_34`, network_channel_incoming_read_item's 5th argument) is written by that call but
// never read again here -- the bit-walk below reads back through
// `network_incoming_message_scratch` instead, exactly as Ghidra's own `local_18 = &DAT_00861de0`
// shows. Preserved exactly; `item` is passed through but otherwise unused by this function.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4db180..0x4db304): while the client
// channel's incoming queue (+0xc) holds data, each item is read (network_channel_incoming_read_item, max 0x80000
// bits in EAX) into the scratch buffer 0x861de0 with its bit offset, bit count and sender address, and walked as a
// local bit stream {1, buffer, first bit, byte/bit cursor, last bit, bit count}: while at least 8 bits remain, one
// bit is read (inlined) and network_incoming_item_dispatch gets it with the stream (ECX) and the sender (ESI).
// The previous C flattened the stream into loose integers, so the dispatch (and everything below it) had no
// stream to decode from, and it passed read_item five of its six arguments. Returns the last result (1 when the
// queue was empty from the start).
```

```
#if 0
Original Ghidra decompilation (0x4db180):

bool __cdecl network_game_process_incoming_messages(int param_1)

{
  int iVar1;
  uint uVar2;
  char cVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  bool bVar8;
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

  bVar8 = true;
  while( true ) {
    iVar1 = *(int *)(*(int *)(param_1 + 0xadc) + 0xc);
    if (iVar1 == 0) {
      return bVar8;
    }
    iVar6 = *(int *)(iVar1 + 0xc) - *(int *)(iVar1 + 8);
    if (iVar6 < 0) {
      iVar6 = iVar6 + *(int *)(iVar1 + 0x10);
    }
    if (iVar6 == 0) break;
    local_38 = 0;
    local_3c = 0;
    cVar3 = FUN_004dcf10(*(int *)(param_1 + 0xadc),&DAT_00861de0,&local_38,&local_3c,local_34);
    bVar8 = false;
    if (cVar3 != '\0') {
      local_c = local_38 & 7;
      local_10 = local_38 >> 3;
      local_8 = local_3c + -1 + local_38;
      local_1c = 1;
      local_18 = &DAT_00861de0;
      uVar5 = local_c;
      uVar4 = local_10;
      local_14 = local_38;
      local_4 = local_3c;
      uVar2 = local_38;
      while ((cVar3 == '\x01' && (7 < ((uVar2 + uVar4 * -8) - uVar5) + local_3c))) {
        bVar8 = false;
        uVar7 = uVar4 * 8 + uVar5;
        local_38 = 0;
        if ((uVar2 <= uVar7) && (uVar7 <= local_8)) {
          local_38 = (int)((uint)(byte)local_18[uVar4] & 1 << ((byte)uVar5 & 0x1f)) >>
                     ((byte)uVar5 & 0x1f) & 0xff;
          uVar7 = uVar7 + 1;
          if (((uVar2 <= uVar7) && (uVar7 <= local_8)) || (uVar7 == local_8 + 1)) {
            uVar5 = uVar7 & 7;
            uVar4 = uVar7 >> 3;
            local_10 = uVar4;
            local_c = uVar5;
          }
          bVar8 = true;
        }
        cVar3 = '\0';
        if (bVar8) {
          cVar3 = FUN_004db630(param_1,local_38);
          uVar5 = local_c;
          uVar4 = local_10;
          uVar2 = local_14;
        }
      }
      bVar8 = cVar3 != '\0';
      local_1c = 0xffffffff;
      local_18 = (undefined *)0x0;
      local_14 = 0;
      local_10 = 0;
      local_c = 0;
      local_8 = 0;
      local_4 = 0;
    }
  }
  return bVar8;
}
#endif
```

## network_game_settings_packet_receive.c

```
// network_game_settings_packet_receive  (Ghidra: FUN_004d9800; renamed, no prior name)
// address 0x4d9800, size 240 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Processes an incoming game-settings/
// map-data packet from the host, storing it locally and applying it the first time it is
// received"). `request+0x68` (dword-indexed, byte 0x1a0) matches network_game_session::
// player_count exactly; `request+0x21` (byte 0x84) matches network_game_session::server_name's
// own relative offset within the session, confirming the comparison is against
// client->session.server_name (byte 0xb98 = client+0xb14+0x84); the whole-session backup/replace
// copies exactly 0xec dwords, matching sizeof(network_game_session)/4 (0x3b0/4 = 236).
// register convention: the client pointer arrives in EBX (unaff_EBX, plain byte-offset this
// time, unlike the word-scaled EBX seen in network_game_settings_packet_send.c);
// `request` is the sole cdecl stack parameter. // blam-cc: EBX -> client, stack -> request
// UNSURE: the final saved/restored dword (`local_c`, the 236th of the 236 backed-up dwords)
// covers session's last dword, i.e. unknown_3ac plus its three padding bytes -- preserved as a
// raw dword save/restore rather than split into named sub-fields, to keep the "whole session
// minus this one field" semantics obvious.
```

```
#if 0
Original Ghidra decompilation (0x4d9800):

undefined4 FUN_004d9800(undefined4 *param_1)

{
  byte bVar1;
  byte *pbVar2;
  int iVar3;
  undefined4 uVar4;
  int unaff_EBX;
  byte *pbVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  bool bVar8;
  undefined4 local_3b8 [235];
  undefined4 local_c;

  if ((*(short *)(param_1 + 0x68) < 0) || (0x10 < *(short *)(param_1 + 0x68))) {
    uVar4 = 0;
  }
  else {
    pbVar2 = (byte *)(param_1 + 0x21);
    pbVar5 = (byte *)(unaff_EBX + 0xb98);
    do {
      bVar1 = *pbVar2;
      bVar8 = bVar1 < *pbVar5;
      if (bVar1 != *pbVar5) {
LAB_004d9864:
        iVar3 = (1 - (uint)bVar8) - (uint)(bVar8 != 0);
        goto LAB_004d9869;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar2[1];
      bVar8 = bVar1 < pbVar5[1];
      if (bVar1 != pbVar5[1]) goto LAB_004d9864;
      pbVar2 = pbVar2 + 2;
      pbVar5 = pbVar5 + 2;
    } while (bVar1 != 0);
    iVar3 = 0;
LAB_004d9869:
    if (iVar3 != 0) {
      main_queue_map_change_by_name_or_clear();
      if (DAT_00718f8c != 1) {
        if ((DAT_00718f8c != 2) && (DAT_00718f8c == 4)) {
          DAT_0068e688 = 0xffffffff;
        }
        DAT_00718f8c = 8;
      }
    }
    puVar6 = (undefined4 *)(unaff_EBX + 0xb14);
    puVar7 = local_3b8;
    for (iVar3 = 0xec; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar7 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar7 = puVar7 + 1;
    }
    puVar6 = (undefined4 *)(unaff_EBX + 0xb14);
    for (iVar3 = 0xec; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *param_1;
      param_1 = param_1 + 1;
      puVar6 = puVar6 + 1;
    }
    *(undefined4 *)(unaff_EBX + 0xec0) = local_c;
    uVar4 = 1;
    if (*(char *)(unaff_EBX + 0xee2) == '\0') {
      FUN_004d9f50();
      *(undefined1 *)(unaff_EBX + 0xee2) = 1;
      return 1;
    }
  }
  return uVar4;
}
#endif
```

## network_game_settings_packet_send.c

```
// network_game_settings_packet_send  (Ghidra: FUN_004d94c0; renamed, no prior name)
// address 0x4d94c0, size 568 bytes
// name confidence: 0.4   rewrite confidence: 0.2 (LOW -- see UNSURE notes)
// evidence: out/phase4/networking_functions.md summary ("Builds and sends a large
// game-settings/map-data packet (including the player's name) to a newly-joining client whose
// session id doesn't yet match ours"). Word-indexed offsets on `unaff_EBX` (an `undefined2 *`)
// resolve cleanly against types/networking.h's network_client_globals when doubled: word 0x76d
// (byte 0xeda) is state, word 0x76f (byte 0xede) is unknown_ede, word 0x5cc (byte 0xb98) is
// exactly &client->session.server_name, word 0x56e (byte 0xadc) is channel, word 0x771 (byte
// 0xee2) is pad_ee2, word 0x788 (byte 0xf10) is unknown_f10.
// register convention: the client pointer arrives in EBX (unaff_EBX); `param_1` is the sole
// cdecl stack parameter (the incoming request record). // blam-cc: EBX -> client, stack -> request
// UNSURE (major): Ghidra's own decompile carries a "Function: __chkstk replaced with injection:
// alloca_probe" warning, and the local-variable list it recovered cannot be correct: a loop
// copies 0x7ff dwords (8188 bytes) into what it calls the 4-byte local `local_2008`, which is
// far larger than the ~0x1ac-byte frame the other named locals imply. This is modeled here as
// one shared byte buffer (`frame`) sized to hold the largest offset actually touched, with every
// named Ghidra local placed at its byte delta from `local_2098` (the lowest-addressed local);
// `local_1eee`, whose Ghidra offset falls inside the big copy's destination range, is read back
// out of the copied template rather than treated as a separately-initialized variable, since
// nothing in the decompiled body ever writes it directly -- this is the only reading of the
// layout consistent with the code as shown.
// UNSURE: `network_prepare_challenge_packet` is called with no visible argument; since every
// local this function builds (`frame`) is otherwise unused after being populated, `&frame[0]`
// is reconstructed as its argument, matching the same shape as network_session_info_packet_send.c.
// UNSURE: word offset 0x7a6 (byte 0xf4c) is exactly `sizeof(network_client_globals)` -- one byte
// past the end of the struct. Preserved as a raw read at that fixed address (very likely landing
// on whatever global the original linker placed immediately after `network_client_storage`),
// not folded into any named field.
// UNSURE: `gcd_compute_response` (0x617c70, GameSpy-adjacent range, not in this batch) and the globals
// `DAT_007461a8` / `DAT_00712dd8` (a large template table) / `DAT_0068e688` are not declared in
// types/networking.h; named generically below.
// UNSURE: the byte-at-a-time strcmp-shaped loop's sign convention (`(1-less)-(less!=0)`) is
// preserved as literal arithmetic rather than replaced with a library strcmp call, since the
// original never calls one here.

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
Original Ghidra decompilation (0x4d94c0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004d94c0(int param_1)

{
  byte bVar1;
  char cVar2;
  int iVar3;
  ushort *puVar4;
  undefined2 *unaff_EBX;
  byte *pbVar5;
  undefined4 *puVar6;
  byte *pbVar7;
  undefined4 *puVar8;
  int iVar9;
  bool bVar10;
  undefined4 local_2098 [4];
  wchar_t local_2088 [8];
  undefined2 local_2078;
  undefined1 local_2076 [73];
  undefined1 local_202d;
  wchar_t local_202a [11];
  undefined2 local_2014;
  undefined2 local_2012;
  undefined2 local_2010;
  undefined1 local_200e;
  undefined1 local_200d;
  undefined1 local_200c;
  undefined1 local_200b;
  undefined4 local_2008;
  undefined2 local_1eee;
  undefined4 uStack_c;

  uStack_c = 0x4d94d0;
  if ((*(byte *)(unaff_EBX + 0x76f) & 2) == 0) {
    unaff_EBX[0x76d] = 2;
    *unaff_EBX = *(undefined2 *)(param_1 + 0xc);
    pbVar7 = (byte *)(param_1 + 0x14);
    pbVar5 = (byte *)(unaff_EBX + 0x5cc);
    do {
      bVar1 = *pbVar7;
      bVar10 = bVar1 < *pbVar5;
      if (bVar1 != *pbVar5) {
LAB_004d9524:
        iVar3 = (1 - (uint)bVar10) - (uint)(bVar10 != 0);
        goto LAB_004d9529;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar7[1];
      bVar10 = bVar1 < pbVar5[1];
      if (bVar1 != pbVar5[1]) goto LAB_004d9524;
      pbVar7 = pbVar7 + 2;
      pbVar5 = pbVar5 + 2;
    } while (bVar1 != 0);
    iVar3 = 0;
LAB_004d9529:
    if ((iVar3 != 0) && (main_queue_map_change_by_name_or_clear(), DAT_00718f8c != 1)) {
      if ((DAT_00718f8c != 2) && (DAT_00718f8c == 4)) {
        DAT_0068e688 = 0xffffffff;
      }
      DAT_00718f8c = 8;
    }
    puVar6 = local_2098;
    for (iVar3 = 0x23; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    *(undefined2 *)puVar6 = 0;
    local_2098[0] = *(undefined4 *)(unaff_EBX + 0x581);
    local_2098[1] = *(undefined4 *)(unaff_EBX + 0x583);
    local_2098[2] = *(undefined4 *)(unaff_EBX + 0x585);
    local_2098[3] = *(undefined4 *)(unaff_EBX + 0x587);
    *(undefined1 *)(unaff_EBX + 0x771) = 0;
    _wcsncpy(local_2088,unaff_EBX + 0x578,8);
    local_202d = *(undefined1 *)(unaff_EBX + 0x7a6);
    local_2078 = 0;
    FUN_00617c70(DAT_007461a8,param_1,local_2076);
    puVar6 = &DAT_00712dd8;
    puVar8 = &local_2008;
    for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar8 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar8 = puVar8 + 1;
    }
    local_200e = *(undefined1 *)(param_1 + 0xc);
    local_200d = 0;
    _wcsncpy(local_202a,(wchar_t *)((int)&local_2008 + 2),0xb);
    local_200c = *(undefined1 *)(unaff_EBX + 0x788);
    local_2014 = 0;
    local_2012 = local_1eee;
    local_2010 = 0xffff;
    local_200b = 0xff;
    *(undefined1 *)(unaff_EBX + 0x771) = 1;
    puVar4 = (ushort *)network_prepare_challenge_packet();
    if (puVar4 != (ushort *)0x0) {
      iVar3 = *(int *)(unaff_EBX + 0x56e);
      iVar9 = (uint)(*puVar4 >> 4) * 8;
      if ((*(byte *)(iVar3 + 0xa8c) & 1) == 0) {
        if ((((*(int *)(iVar3 + 0x24) + *(int *)(iVar3 + 0x1c) * -8) - *(int *)(iVar3 + 0x20)) + 1 <
             iVar9 + 1) && (cVar2 = FUN_004ddb60(iVar3,1), cVar2 == '\0')) {
          return;
        }
        *(int *)(iVar3 + 0xa80) = *(int *)(iVar3 + 0xa80) + iVar9 + 1;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar3 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar9);
        *(undefined1 *)(iVar3 + 0x2c) = 0;
      }
      *(byte *)(unaff_EBX + 0x76f) = *(byte *)(unaff_EBX + 0x76f) | 2;
    }
  }
  return;
}
#endif
```

## network_game_state_update_receive.c

```
// network_game_state_update_receive  (Ghidra: FUN_004d9d20; renamed, no prior name)
// address 0x4d9d20, size 261 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Processes an incoming sequenced
// game-state update packet, growing the per-connection reassembly buffer as needed and handing
// the payload off for application"). client+0xecc/+0xed0 match types/networking.h's
// network_client_globals::unknown_ecc/unknown_ed0 exactly.
// register convention: __cdecl-shaped stack parameters (client, the incoming record) per
// Ghidra's own recovery. // blam-cc: stack -> client, record
// UNSURE: client+0xcb4 lands at session-relative offset 0x1a0, exactly
// network_game_session::player_count. Used here as a reassembly-buffer growth target, which is
// surprising for a field named "player count"; accessed via `client->session.player_count`
// as declared rather than invented as a separate field, per the task's rule against redefining
// header fields.
// UNSURE: `record`'s own layout (a dword sequence number at +0x00, two more dwords at +0x04/+0x08,
// a 16-bit capacity at +0x0e, and an 8-dword-stride payload array from +0x10) has no declared
// type anywhere; accessed via raw offsets on a `uint32_t *`/`uint8_t *` view.
// UNSURE: `DAT_006f1d6c` sits 0x4c bytes into types/networking.h's documented "network game
// engine callback block" (0x006f1d20), but the header does not resolve interior offsets of that
// block; declared here as its own opaque global at its literal address instead.
// UNSURE: the growth loop's `(target - current) & 0x7ffffff` mask and the two back-to-back
// zero-fill loops (the second of which never executes, since its own counter is initialized to
// 0) are preserved exactly, including the dead second loop.
```

```
#if 0
Original Ghidra decompilation (0x4d9d20):

undefined4 FUN_004d9d20(int param_1,uint *param_2)

{
  short sVar1;
  undefined4 uVar2;
  int iVar3;
  uint *puVar4;
  uint *puVar5;
  undefined8 uVar6;
  LARGE_INTEGER local_318;
  ushort local_310;
  uint local_30c [194];

  sVar1 = *(short *)((int)param_2 + 0xe);
  if (sVar1 < *(short *)(param_1 + 0xcb4)) {
    puVar4 = param_2 + sVar1 * 8 + 4;
    for (iVar3 = ((int)*(short *)(param_1 + 0xcb4) - (int)sVar1 & 0x7ffffffU) << 3; iVar3 != 0;
        iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    for (iVar3 = 0; iVar3 != 0; iVar3 = iVar3 + -1) {
      *(undefined1 *)puVar4 = 0;
      puVar4 = (uint *)((int)puVar4 + 1);
    }
    *(undefined2 *)((int)param_2 + 0xe) = *(undefined2 *)(param_1 + 0xcb4);
  }
  if ((*param_2 <= *(uint *)(param_1 + 0xecc)) ||
     (((DAT_0071c2d4 == 0 && (*(uint *)(DAT_006f1d6c + 0xc) == param_2[2])) &&
      (param_2[1] != random_seed_global)))) {
    FUN_004d9340();
  }
  local_310 = *(ushort *)((int)param_2 + 0xe);
  puVar4 = param_2 + 4;
  puVar5 = local_30c;
  for (iVar3 = (uint)local_310 << 3; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  for (iVar3 = 0; iVar3 != 0; iVar3 = iVar3 + -1) {
    *(char *)puVar5 = (char)*puVar4;
    puVar4 = (uint *)((int)puVar4 + 1);
    puVar5 = (uint *)((int)puVar5 + 1);
  }
  FUN_004734b0();
  *(uint *)(param_1 + 0xecc) = *param_2;
  QueryPerformanceCounter(&local_318);
  uVar6 = __allmul(local_318.s.LowPart,local_318.s.HighPart,1000,0);
  uVar2 = __alldiv(uVar6,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(param_1 + 0xed0) = uVar2;
  return 1;
}
#endif
```

## network_incoming_item_dispatch.c

```
// network_incoming_item_dispatch  (Ghidra: FUN_004db630; renamed, no prior name)
// address 0x4db630, size 121 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (REWRITTEN; was 0.4)
// evidence: out/phase4/networking_functions.md summary ("Dispatches a single incoming queue
// entry either to the queued-game-action applier or to the network-message decode switch,
// depending on an entry-type flag").
// register convention: Ghidra's own recovered stack parameters (client, item_flag); ESI is also
// a genuine live-in, pushed as network_game_action_queue_drain's own `expected_sequence` stack
// argument (objdump 0x4db652 `push esi`, read with no local setup).
// blam-cc: ECX -> stream, ESI -> sender, stack -> client, item_flag
// FIXED (register inputs, objdump): ESI is a genuine live-in the notes did not map; added as
// `expected_sequence` and forwarded instead of the hardcoded pointer-to-zero local. While
// tracing that call, also found network_game_message_decode_dispatch's own file
// (src/networking/network_game_message_decode_dispatch.c) already recovered its real signature
// as (uint16_t *record /*EDX*/, int32_t record_length /*EDI*/) -- not `(client)` as this file's
// stale extern claimed -- and this function was discarding network_message_read_sized_buffer's
// return value (the record pointer) instead of forwarding it; both fixed below to match
// objdump 0x4db680-0x4db68b (movzx edi,[eax]; mov edx,eax; shr edi,4).
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4db630..0x4db6a9): the item's stream
// arrives in ECX and the sender address in ESI. A game-action item (flag 1) goes to
// network_game_action_queue_drain(client, stream, sender); a message item (flag 0) is read into a local 0x1000-byte
// buffer (network_message_read_sized_buffer: EDI buffer, EBX stream, capacity 0xfff) and handed to
// network_game_message_decode_dispatch with the client (EAX), the record (EDX), its length (the first word >> 4,
// EDI) and the sender (stack).

// VERIFIED against disassembly 0x4db630..0x4db6a9 (2026-09-30): flag 1 -> drain(client, stream, sender); flag 0 -> read_sized_buffer(EDI buffer, EBX/ECX stream, 0xfff) then decode_dispatch(EAX client, EDX record, EDI length, stack sender); else 0
```

```
#if 0
Original Ghidra decompilation (0x4db630):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_004db630(undefined4 param_1,uint param_2)

{
  uint uVar1;
  int iVar2;

  if (param_2 == 1) {
    uVar1 = FUN_004db870(param_1);
    return uVar1;
  }
  if (param_2 == 0) {
    iVar2 = FUN_004de420(0xfff);
    param_2 = 0;
    if (iVar2 != 0) {
      uVar1 = network_game_message_decode_dispatch();
      return uVar1;
    }
  }
  return param_2 & 0xffffff00;
}
#endif
```
