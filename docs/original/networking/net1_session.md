# Original notes: networking `net1_session`

Author notes and decompile blocks moved verbatim from the C files converted into the net1_session sources.

## network_game_broadcast_team_object_updates.c

```
// network_game_broadcast_team_object_updates  (Ghidra: FUN_004df950, unnamed)
// address 0x4df950, size 183 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Walks every live game object and, for the
// ones belonging to an active team, encodes and broadcasts an object-update packet,
// returning the total bytes sent and object count through param_2/unaff_EDI." Iterates with
// object_iterator_next (types/objects.h object_iterator, already rewritten at
// src/objects/object_iterator_next.c); object+4 matches types/objects.h object::network_role;
// object+0xb4 matches object::type.
// register convention: EDI = object_count (int32_t *), stack = param_1 (unused), param_2
// (int32_t *bytes_sent).
// blam-cc: EDI -> object_count, stack -> (unused, bytes_sent)
// UNSURE: the iterator's type_mask (object_iterator+0x00) is never assigned a value in the
// decompiled body; treated as "every type" (0xffffffff) to match the summary's "every live
// game object". flags_mask is assigned 0, which passes every object regardless of flags.
// UNSURE: `(&PTR_PTR_0069bfdc)[object->type]` indexes an unresolved per-object-type table;
// declared here as an opaque pointer array and the +0x10 read is transcribed literally
// without a named field, since no struct for that table exists in types/objects.h.
// UNSURE: FUN_004f44f0's real parameters/purpose (object-update encode into a shared
// 0x7ff8-byte scratch buffer) are not independently confirmed here.
// UNSURE: param_1 is a genuine but entirely unused stack parameter in the original.
```

```
#if 0
Original Ghidra decompilation (0x4df950):

void FUN_004df950(undefined4 param_1,int *param_2)

{
  int iVar1;
  int *unaff_EDI;
  undefined1 local_10 [4];
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar1 = object_iterator_next(local_10);
  while (iVar1 != 0) {
    if (((*(int *)(iVar1 + 4) == 0) &&
        (*(int *)((&PTR_PTR_0069bfdc)[*(short *)(iVar1 + 0xb4)] + 0x10) != -1)) &&
       (iVar1 = FUN_004f44f0(&DAT_00871de0,0x7ff8), 0 < iVar1)) {
      *param_2 = *param_2 + iVar1;
      *unaff_EDI = *unaff_EDI + 1;
      network_session_send_to_machine(1,&DAT_00871de0,iVar1,1,0,0,3);
    }
    iVar1 = object_iterator_next(local_10);
  }
  return;
}
#endif
```

## network_game_client_apply_position_update.c

```
// network_game_client_apply_position_update  (Ghidra: FUN_004dff70, unnamed)
// address 0x4dff70, size 264 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Applies a received position/orientation
// delta packet to the local object via FUN_00473390, after validating the packet's tick and
// delta-count fields against the destination's tag data (datum_get)." 0x006b1460 matches
// types/game.h's `machine_to_player[16]`; the datum_get idiom (index/salt check against
// player_data) matches src/memory/datum_get.c exactly; player+0x11c matches types/game.h's
// player::unknown_11c.
// register convention: all four parameters are genuine stack (cdecl) parameters per Ghidra's
// own signature for this function.
// blam-cc: stack -> state, packet, param_3, object
// UNSURE: `state` (param_1) has no named type; its two touched fields (a tick at +0x04 and a
// machine-index byte pair at +0x0c) line up with the 0x34-byte network_machine::connect_state
// block that network_game_client_apply_received_update.c stages into a local copy before
// calling this function with no visible arguments -- so `state` is very likely a pointer into
// (or the base of) that staged copy, but connect_state itself is documented as fully opaque
// in out/phase4/networking_types_notes.md, so no field names are invented for it here; raw
// offsets are used instead.
// FIXED (verified against 0x4e0017..0x4e0028): FUN_00473390 is update_server_queue_push_history(AX machine,
//   EDX tick count = param_3, stack: delta record, param_4); param_3 is read after all.
```

```
#if 0
Original Ghidra decompilation (0x4dff70):

void FUN_004dff70(int param_1,uint *param_2,undefined4 param_3,undefined4 param_4)

{
  short sVar1;
  int iVar2;
  uint *puVar3;
  uint *puVar4;
  uint local_40 [16];

  if (((*(uint *)(param_1 + 4) <= (*param_2 & 0x7fffffff)) &&
      (sVar1 = *(short *)((int)param_2 + 6), -1 < sVar1)) && (sVar1 < 2)) {
    local_40[1] = 0;
    local_40[2] = 0;
    local_40[3] = 0;
    local_40[4] = 0;
    local_40[5] = 0;
    local_40[6] = 0;
    local_40[7] = 0;
    local_40[0] = 0;
    if (0 < sVar1) {
      puVar3 = param_2 + 2;
      puVar4 = local_40;
      for (iVar2 = ((int)sVar1 & 0x7ffffffU) << 3; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar4 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar4 = puVar4 + 1;
      }
    }
    if ((((&DAT_006b1460)[*(ushort *)(param_1 + 0xc)] != -1) && (iVar2 = datum_get(), iVar2 != 0))
       && (*(int *)(iVar2 + 0x34) != -1)) {
      FUN_00473390(local_40,param_4);
      *(uint *)(param_1 + 4) = *param_2 & 0x7fffffff;
      puVar3 = local_40;
      puVar4 = local_40 + 8;
      for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar4 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar4 = puVar4 + 1;
      }
      iVar2 = datum_get();
      if (iVar2 != 0) {
        *(uint *)(iVar2 + 0x11c) = local_40[8] & 0x4d0;
      }
    }
  }
  return;
}
#endif
```

## network_game_client_apply_received_update.c

```
// network_game_client_apply_received_update  (Ghidra: network_game_client_apply_received_update,
// already named)
// address 0x4e0280, size 308 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Processes one received network update packet
// for the given connection: updates timing/history bookkeeping, checks client latency via
// network_client_check_connection_quality, applies the position delta with FUN_004dff70, and log[s]..." unaff_EBX+0x1c is
// exactly network_machine::connect_state (13 dwords, 0x34 bytes); the log format string
// ("[%d]: [%d]:\t Received update [%d] for [%d] ticks.\n") and unaff_EBX+0xc (byte within
// connect_state) match network_player_update_history_log_write's summary.
// register convention: EBX = machine (network_machine *), stack = param_1 (unused),
// message (a pointer to a pointer to the message record).
// blam-cc: EBX -> machine, stack -> (unused, message)
// UNSURE: no caller in this batch shows EBX being loaded before this call (it is presumably
// set up by an outer caller outside the range being rewritten here); modelled as a normal
// parameter per the register convention regardless.
// UNSURE: connect_state is documented as fully opaque in out/phase4/networking_types_notes.md.
// It is staged into a local copy before the type-1/else dispatch and copied back afterward, so
// message_delta_read_changed_subfields and FUN_004ec590 almost certainly take that staged copy
// as an implicit decode-context argument, but neither call shows any visible arguments in the
// decompilation; both are called argument-less here, matching Ghidra literally.
// UNSURE: network_client_check_connection_quality and network_game_client_apply_position_update (FUN_004dff70) are
// likewise called with no visible arguments in the original. The latter's real signature (see
// network_game_client_apply_position_update.c) needs a `state` and a `packet` pointer, which
// are very likely the staged connect_state copy and the resolved message record respectively;
// its `object` argument has no visible source at all and is passed as NULL here rather than
// invented.
// UNSURE: the message record's own layout is not established; accessed only through the two
// offsets this function itself touches (type at +0x00, a counter at +0x0c, a flag byte at
// +0x1d), consistent with out/phase4/networking_types_notes.md's note that the message-delta
// wire record has no declared type in this module.
```

```
#if 0
Original Ghidra decompilation (0x4e0280):

void network_game_client_apply_received_update(undefined4 param_1,int *param_2)

{
  char cVar1;
  int iVar2;
  int unaff_EBX;
  undefined4 *puVar3;
  char *pcVar4;
  undefined4 auStackY_9c [7];
  undefined4 uStackY_80;
  char local_34 [52];

  puVar3 = (undefined4 *)(unaff_EBX + 0x1c);
  pcVar4 = local_34;
  for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
    *(undefined4 *)pcVar4 = *puVar3;
    puVar3 = puVar3 + 1;
    pcVar4 = pcVar4 + 4;
  }
  param_2 = (int *)*param_2;
  if (*param_2 == 1) {
    iVar2 = message_delta_read_changed_subfields();
    param_2[3] = param_2[3] + iVar2;
    *(undefined1 *)((int)param_2 + 0x1d) = 1;
  }
  else {
    FUN_004ec590();
  }
  pcVar4 = local_34;
  puVar3 = (undefined4 *)(unaff_EBX + 0x1c);
  for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = *(undefined4 *)pcVar4;
    pcVar4 = pcVar4 + 4;
    puVar3 = puVar3 + 1;
  }
  if (local_34[0] != '\0') {
    pcVar4 = local_34;
    puVar3 = auStackY_9c;
    for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = *(undefined4 *)pcVar4;
      pcVar4 = pcVar4 + 4;
      puVar3 = puVar3 + 1;
    }
    cVar1 = FUN_004e0080();
    if (cVar1 == '\x01') {
      FUN_004dff70();
      if (*(short *)(unaff_EBX + 0xc) != 0) {
        GetTickCount();
        uStackY_80 = 0x4e03aa;
        network_player_update_history_log_write
                  ("[%d]: [%d]:\t Received update [%d] for [%d] ticks.\n");
      }
    }
  }
  return;
}
#endif
```

## network_game_generate_unique_random_name.c

```
// network_game_generate_unique_random_name  (Ghidra: network_game_generate_unique_random_name,
// already named)
// address 0x4df730, size 96 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Picks a random default player name via
// network_game_get_random_player_name(), retrying until it doesn't collide with any currently
// active player, and copies the result into the output buffer." Same session scan base
// (container + 0x1aa == session.players[0].name, stride 0x10 wchars == 0x20 bytes) as
// network_player_name_collision_check.c.
// register convention: session container in EAX (in_EAX). blam-cc: EAX -> session, stack ->
// out_name
```

```
#if 0
Original Ghidra decompilation (0x4df730):

void network_game_generate_unique_random_name(wchar_t *param_1)

{
  char cVar1;
  int in_EAX;
  wchar_t *_Str2;
  int iVar2;
  int iVar3;
  wchar_t *_Str1;
  int iVar4;

  do {
    _Str2 = (wchar_t *)network_game_get_random_player_name();
    iVar3 = 0;
    iVar4 = 0x10;
    _Str1 = (wchar_t *)(in_EAX + 0x1aa);
    do {
      cVar1 = FUN_004de9f0();
      if (cVar1 != '\0') {
        iVar2 = _wcscmp(_Str1,_Str2);
        if (iVar2 == 0) {
          iVar3 = iVar3 + 1;
        }
      }
      _Str1 = _Str1 + 0x10;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  } while (iVar3 != 0);
  _wcsncpy(param_1,_Str2,0xb);
  param_1[0xb] = L'\0';
  return;
}
#endif
```

## network_game_get_random_player_name.c

```
// network_game_get_random_player_name  (Ghidra: network_game_get_random_player_name, already
// named)
// address 0x4dea80, size 112 bytes
// name confidence: 0.7   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Looks up the 'ui\random_player_names' tag and,
// if it has entries, returns a randomly selected default player name; otherwise returns the
// built-in fallback name string." tag_instances (0x0087bc14) and its `(index*0x20+0x14)`
// definition-pointer idiom match the same pattern already established throughout src/ai (e.g.
// actor_apply_unit_definition_properties.c); neither `tag_instance` nor the unicode_string_list
// tag body has a declared type anywhere in types/tags.h, so this rewrite keeps the raw-offset
// form rather than inventing one.
// UNSURE: text_string_list_get_string's index argument is elided at this call site (matching the
// widespread "tag_lookup/text_string_list_get_string pair called with no visible index" issue
// already documented in src/game/game_engine_build_end_game_result_text.c); reconstructed as the
// LCG-derived random value, modulo the list's own count, though that modulo is not directly
// visible in the decompile either.
```

```
#if 0
Original Ghidra decompilation (0x4dea80):

undefined * network_game_get_random_player_name(void)

{
  uint uVar1;
  undefined *puVar2;

  uVar1 = tag_lookup("ui\\random_player_names");
  if ((uVar1 != 0xffffffff) && (*(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) != 0)) {
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    puVar2 = (undefined *)text_string_list_get_string();
    return puVar2;
  }
  return &DAT_00660c34;
}
#endif
```

## network_game_is_active.c

```
// network_game_is_active  (Ghidra: network_game_is_active, already named)
// address 0x4ddca0, size 27 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/networking_functions.md: "Returns whether a network game is currently
// active by checking that either the network-game globals or the host globals pointer is
// non-null." network_client (0x0071c2d8) and network_server (0x0071c2d4) match
// types/networking.h exactly.
```

```
#if 0
Original Ghidra decompilation (0x4ddca0):

int __cdecl network_game_is_active(void)

{
  if ((DAT_0071c2d8 == 0) && (DAT_0071c2d4 == 0)) {
    return 0;
  }
  return 1;
}
#endif
```

## network_game_process_incoming_message.c

```
// network_game_process_incoming_message  (Ghidra: network_game_process_incoming_message,
// already named)
// address 0x4e1c60, size 614 bytes
// name confidence: 0.7   rewrite confidence: 0.85 (REWRITTEN; was 0.35)
// evidence: out/phase4/networking_functions.md: "Top-level decoder/dispatcher for incoming
// 'network game' protocol messages, decoding each message with the network-game message group
// and routing it by type byte." The bitstream-header check (`(*record & 3) == 0 && ((*record
// >> 2) & 3) == 3`) matches network_game_message_decode_dispatch.c's identical check; `in_ECX`'s
// offset+0xe matches network_machine::flags/unknown_0f, and `param_1` is passed straight through
// to FUN_004e21d0 (this batch), whose own header confirms it as `server`.
// register convention: EAX = length, ECX = machine, EDX = record, stack = server. Verified case
// by case against the raw switch table below (Ghidra's own decompile groups two case labels
// under one call for two pairs -- `case 0x14: case 0x25:` both call FUN_004e26a0, `case 0x1a`
// alone calls FUN_004e2700 -- these are trusted over the low-confidence per-function summaries
// in networking_functions.md, three of which had their type numbers swapped; see the affected
// files' own UNSURE notes).
//   // blam-cc: EAX -> length, ECX -> machine, EDX -> record, stack -> server
// UNSURE (major): every case in the switch below that Ghidra shows calling its handler with no
// visible arguments is modelled the same way network_game_message_decode_dispatch.c models its
// own 18-handler switch: through a raw zero-argument function-pointer cast, matching Ghidra's
// literal CALL with whatever registers happen to already be live, rather than forcing each
// handler's independently-reconstructed (and non-uniform) parameter list onto this dispatcher.
// UNSURE: cases 1 and 0x1b decode and act inline rather than delegating to a named handler;
// FUN_004e2110 (this batch) is called with the address of the just-decoded record, which is the
// one case where a real argument is visible; FUN_004dff70 (network_game_client_apply_position_update,
// prior batch) is called with none.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e1c60: EAX length, ECX machine, EDX record, stack server; jump tables 0x4e1f0c / 0x4e1ec8. The message
// type is the record's last byte; a machine without flag bit 1 only gets type 0xe, or type 1 with bit 4. Every
// handler now receives what the original passes it (the previous C passed nothing to eleven of them).
```

```
#if 0
Original Ghidra decompilation (0x4e1c60), from tools/pack.py 0x4e1c60:

undefined4 network_game_process_incoming_message(int param_1)

{
  char cVar1;
  int in_EAX;
  undefined4 uVar2;
  byte bVar3;
  int in_ECX;
  ushort *in_EDX;
  char local_38 [4];
  uint local_34;
  int local_30;
  int local_2c;
  undefined1 local_28 [40];

  if ((((*in_EDX & 3) == 0) && (bVar3 = (byte)*in_EDX >> 2 & 3, bVar3 != 1)) && (bVar3 == 3)) {
    local_38[0] = *(char *)((short)in_EAX + -1 + (int)in_EDX);
    local_34 = (uint)*(ushort *)(in_ECX + 0xe);
    bVar3 = (byte)*(ushort *)(in_ECX + 0xe);
    if ((((bVar3 >> 1 & 1) != 0) || (local_38[0] == '\x0e')) ||
       (((bVar3 >> 4 & 1) != 0 && (local_38[0] == '\x01')))) {
      switch(local_38[0]) {
      case '\x01':
        if (DAT_0071c2dc != '\0') {
          local_30 = in_EAX + -2;
          cVar1 = data_packet_group_decode_packet
                            (&PTR_s_network_game_messages_group_006994f8,&local_2c,in_EDX + 1,
                             &local_34,local_38,0);
          if (cVar1 != '\0') {
            FUN_004e2110(&local_2c);
            return 1;
          }
        }
        break;
      case '\x0e':
        uVar2 = FUN_004e21d0(param_1);
        return uVar2;
      case '\x0f':
        uVar2 = FUN_004e2400();
        return uVar2;
      case '\x10':
        uVar2 = FUN_004e24d0();
        return uVar2;
      case '\x11':
        uVar2 = FUN_004e2530();
        return uVar2;
      case '\x12':
        uVar2 = FUN_004e2580();
        return uVar2;
      case '\x13':
        uVar2 = FUN_004e25e0();
        return uVar2;
      case '\x14':
      case '%':
        uVar2 = FUN_004e26a0();
        return uVar2;
      case '\x15':
        uVar2 = FUN_004e2630();
        return uVar2;
      case '\x1a':
        uVar2 = FUN_004e2700();
        return uVar2;
      case '\x1b':
        if (*(short *)(param_1 + 4) == 1) {
          local_2c = in_EAX + -2;
          cVar1 = data_packet_group_decode_packet
                            (&PTR_s_network_game_messages_group_006994f8,local_28,in_EDX + 1,
                             local_38,&local_34,5);
          if (cVar1 != '\0') {
            FUN_004dff70();
          }
        }
        break;
      case '\x1c':
        uVar2 = FUN_004e2790(param_1);
        return uVar2;
      case '\x1d':
        uVar2 = FUN_004e2810();
        return uVar2;
      case '\x1e':
        uVar2 = FUN_004e2870();
        return uVar2;
      case '#':
        uVar2 = FUN_004e28d0();
        return uVar2;
      case '$':
        uVar2 = FUN_004e2930();
        return uVar2;
      }
    }
  }
  return 1;
}
#endif
```

## network_game_scenario_load_request.c

```
// network_game_scenario_load_request  (Ghidra: network_game_scenario_load_request, already
// named)
// address 0x4de6d0, size 405 bytes
// name confidence: 0.55   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Prepares and issues a scenario_load() request
// for the network game using the requested map name and seed, then, when hosting, opens a
// channel for every connected machine and returns whether the map is now loaded." session at
// param_1: server_name (+0x84), unknown_19e (+0x19e) and unknown_3ac (+0x3ac, the map-loaded
// flag already established in network_host_shutdown_or_defer.c) all match
// types/networking.h's network_game_session.
// FIXED in the review pass: the first draft of this file declared the staged record with
// map_name at offset 0 and a 132-byte prefix afterwards, which put every field at the wrong
// place. Ghidra's own locals (local_110 dword, local_10c word, local_10a word, local_108 dword,
// local_104[260]) and the two 0x43-dword (0x10c byte) loops that zero and copy the record pin
// the real layout: seed at +0x06, salt at +0x08, map_name at +0x0c, total size 0x10c. The
// struct now lives in types/networking.h as network_scenario_load_request.
// +0x3a4 (the session-relative "salt", also touched by network_game_server_host_create.c) is kept
// as a raw offset; +0x134 is session->variant + 0x30 (the variant's engine index).
// DAT_006b0b80 (main_game_globals) is the scenario_load staging area (request copied to +8).
// register convention: fully recovered cdecl (session is the only parameter).
// reconciled: R33 game_time_globals.unknown_00 -> initialized (uint8 at +0x00, same byte)
// reconciled: R13 network_scenario_load_request.seed (+0x06) -> difficulty (campaign difficulty, lands at game globals +0x0e)

// VERIFIED against disassembly 0x4de6d0..0x4de865 (2026-09-30): FIXED: cache_file_switch_map_by_path(EAX = request.map_name, EBX = 1) x2, game_engine_apply_variant(EDX = &session->variant) and scenario_load(EAX = request.map_name) were called without their register arguments (scenario_load was handed main_game_globals); order of calls, the mode 1/2/3 salt selection, player loop and host tick-record tail compared. The null guard on the shared session is an addition (the original dereferences it unchecked)
```

```
#if 0
Original Ghidra decompilation (0x4de6d0):

char __cdecl network_game_scenario_load_request(int param_1)

{
  char cVar1;
  uint uVar2;
  int iVar3;
  undefined4 *puVar4;
  char *pcVar5;
  undefined4 local_110;
  undefined2 local_10c;
  undefined2 local_10a;
  undefined4 local_108;
  char local_104 [260];

  puVar4 = &local_110;
  for (iVar3 = 0x43; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  local_10c = 0;
  local_10a = 1;
  local_108 = 0xdeadbeef;
  _strncpy(local_104,(char *)(param_1 + 0x84),0x7f);
  local_10a = *(undefined2 *)(param_1 + 0x19e);
  if (0 < DAT_00719720) {
    if (DAT_00719720 < 3) {
      if (DAT_0071c2d4 == 0) {
        if (DAT_0071c2d8 == 0) {
          iVar3 = 0;
        }
        else {
          iVar3 = DAT_0071c2d8 + 0xb14;
        }
      }
      else {
        iVar3 = DAT_0071c2d4 + 8;
      }
      local_108 = *(undefined4 *)(iVar3 + 0x3a4);
    }
    else if (DAT_00719720 == 3) {
      local_108 = *(undefined4 *)(param_1 + 0x3a4);
    }
  }
  FUN_0045aea0();
  if ((*DAT_006f1d6c != '\0') && ((DAT_006f1d6c[1] != '\0' || (DAT_006f1d6c[2] != '\0')))) {
    FUN_0045b370();
    FUN_0045afb0();
  }
  main_menu_music_stop();
  if (*(int *)(param_1 + 0x134) != 0) {
    FUN_0045b990();
  }
  FUN_0045aea0();
  puVar4 = &local_110;
  pcVar5 = DAT_006b0b80 + 8;
  for (iVar3 = 0x43; iVar3 != 0; iVar3 = iVar3 + -1) {
    *(undefined4 *)pcVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    pcVar5 = pcVar5 + 4;
  }
  uVar2 = scenario_load();
  if ((char)uVar2 == '\0') {
    if (*DAT_006b0b80 == '\0') goto LAB_004de854;
  }
  else {
    *DAT_006b0b80 = '\x01';
  }
  *(undefined1 *)(param_1 + 0x3ac) = 1;
  FUN_0045b050();
  if (DAT_00719720 == 2) {
    iVar3 = 0;
    do {
      cVar1 = FUN_004de9f0();
      if (cVar1 == '\0') break;
      cVar1 = FUN_004de870();
      if (cVar1 == '\0') {
        *(undefined1 *)(param_1 + 0x3ac) = 0;
        break;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < 0x10);
    if ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) != 0) {
      FUN_00470ae0();
      FUN_0045b8b0();
    }
  }
LAB_004de854:
  return *(char *)(param_1 + 0x3ac);
}
#endif
```

## network_game_search_entry_is_fresh.c

```
// network_game_search_entry_is_fresh  (Ghidra: FUN_004da770; renamed, no prior name)
// address 0x4da770, size 94 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Tests whether a single entry in the
// 9-slot game-search/pending-connection record table is still within its ~6 second freshness
// window"); entry+0x12d and entry+0x18 match types/networking.h's
// network_game_search_entry::in_use and ::received_ms exactly, and 0x1771 (6001 ms) matches
// k_network_game_search_expiry_ms (6000) plus one.
// register convention: the entry pointer arrives in ESI (unaff_ESI). // blam-cc: ESI -> entry
// Return: only AL is defined (0 or 1; the upper EAX bytes are scratch), so a plain 0/1 uint8_t
// return is equivalent.

// VERIFIED against disassembly 0x4da770..0x4da7ce (2026-09-30): in_use at +0x12d, QPC*1000/freq via _allmul/_alldiv, elapsed (signed) <= 0x1770; AL result only
```

```
#if 0
Original Ghidra decompilation (0x4da770):

uint FUN_004da770(void)

{
  undefined4 in_EAX;
  int iVar1;
  uint uVar2;
  int unaff_ESI;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  uVar2 = CONCAT31((int3)((uint)in_EAX >> 8),*(char *)(unaff_ESI + 0x12d));
  if (*(char *)(unaff_ESI + 0x12d) != '\0') {
    QueryPerformanceCounter(&local_8);
    uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    iVar1 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
    uVar2 = iVar1 - *(int *)(unaff_ESI + 0x18);
    if ((int)uVar2 < 0x1771) {
      return CONCAT31((int3)(uVar2 >> 8),1);
    }
  }
  return uVar2 & 0xffffff00;
}
#endif
```

## network_game_search_results_add_or_update.c

```
// network_game_search_results_add_or_update  (Ghidra: network_game_search_results_add_or_update,
// already named)
// address 0x4da7d0, size 582 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_types_notes.md "network_game_search_entry (0x130)" fully
// documents this function's field writes; every offset below is taken directly from that
// section and from types/networking.h's network_game_search_entry struct.
// register convention: the search table (9-entry array) is the sole cdecl stack parameter
// (`param_1`); the incoming announcement record arrives in EBX (unaff_EBX).
// blam-cc: EBX -> announcement, stack -> results
// UNSURE: `announcement`'s own layout has no declared type (types/networking.h explicitly
// leaves it undeclared -- see that header's note on the message-delta-decoded announcement
// record); accessed via raw offsets on a `uint8_t *` view, matching the type notes' own
// convention.
// Return: only AL is defined (0 or 1), so a plain 0/1 return is equivalent.

// VERIFIED against disassembly 0x4da7d0..0x4daa16 (2026-09-30): FIXED: eviction scans for the first entry with joinable == 0 (+0x12c, cl at 0x4da8a0), not unknown_12f (+0x12f); everything else (expiry, identity match on dword 0, free slot, field copies/offsets, name fallback L"???", flags) matches
```

```
#if 0
Original Ghidra decompilation (0x4da7d0):

uint network_game_search_results_add_or_update(int *param_1)

{
  int iVar1;
  uint uVar2;
  int *piVar3;
  int *unaff_EBX;
  int iVar4;
  int *piVar5;
  int *piVar6;
  undefined8 uVar7;
  wchar_t *_Source;
  char local_9;
  LARGE_INTEGER local_8;

  if (((*(byte *)((int)unaff_EBX + 0x15e) & 2) == 0) ||
     (local_9 = '\x01', 0xf < *(short *)((int)unaff_EBX + 0x156))) {
    local_9 = '\0';
  }
  piVar3 = param_1 + 6;
  iVar4 = 9;
  do {
    if (*(char *)((int)piVar3 + 0x115) == '\0') {
LAB_004da849:
      piVar5 = piVar3 + -6;
      for (iVar1 = 0x4c; iVar1 != 0; iVar1 = iVar1 + -1) {
        *piVar5 = 0;
        piVar5 = piVar5 + 1;
      }
    }
    else {
      QueryPerformanceCounter(&local_8);
      uVar7 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
      iVar1 = __alldiv(uVar7,DAT_006ac8f8,DAT_006ac8fc);
      if (6000 < iVar1 - *piVar3) goto LAB_004da849;
    }
    piVar3 = piVar3 + 0x4c;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  iVar4 = 0;
  piVar3 = param_1;
  do {
    if (*unaff_EBX == *piVar3) goto LAB_004da8ce;
    iVar4 = iVar4 + 1;
    piVar3 = piVar3 + 0x4c;
  } while (iVar4 < 9);
  iVar4 = 0;
  piVar3 = param_1;
  do {
    if (*(char *)((int)piVar3 + 0x12d) == '\0') goto LAB_004da8ce;
    iVar4 = iVar4 + 1;
    piVar3 = piVar3 + 0x4c;
  } while (iVar4 < 9);
  uVar2 = CONCAT31((int3)((uint)iVar4 >> 8),local_9);
  if (local_9 != '\0') {
    uVar2 = 0;
    piVar3 = param_1;
    do {
      if ((char)piVar3[0x4b] == '\0') {
        piVar5 = piVar3;
        for (iVar4 = 0x4c; iVar4 != 0; iVar4 = iVar4 + -1) {
          *piVar5 = 0;
          piVar5 = piVar5 + 1;
        }
LAB_004da8ce:
        *(undefined1 *)((int)piVar3 + 0x12d) = 1;
        *piVar3 = *unaff_EBX;
        piVar3[1] = unaff_EBX[1];
        piVar3[2] = unaff_EBX[2];
        piVar3[3] = unaff_EBX[3];
        piVar3[4] = unaff_EBX[4];
        piVar3[5] = unaff_EBX[5];
        QueryPerformanceCounter(&local_8);
        uVar7 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
        iVar4 = __alldiv(uVar7,DAT_006ac8f8,DAT_006ac8fc);
        piVar3[6] = iVar4;
        _Source = (wchar_t *)((int)unaff_EBX + 0x1e);
        *(short *)((int)piVar3 + 0x12a) = (short)unaff_EBX[7];
        if (*_Source == L'\0') {
          _Source = L"???";
        }
        _wcsncpy((wchar_t *)(piVar3 + 7),_Source,0x3f);
        *(undefined2 *)((int)piVar3 + 0x9a) = 0;
        *(short *)(piVar3 + 0x48) = (short)unaff_EBX[0x55];
        piVar5 = unaff_EBX + 0x34;
        piVar6 = piVar3 + 0x27;
        for (iVar4 = 0x21; iVar4 != 0; iVar4 = iVar4 + -1) {
          *piVar6 = *piVar5;
          piVar5 = piVar5 + 1;
          piVar6 = piVar6 + 1;
        }
        *(undefined2 *)((int)piVar3 + 0x122) = *(undefined2 *)((int)unaff_EBX + 0x156);
        *(short *)(piVar3 + 0x49) = (short)unaff_EBX[0x56];
        *(undefined2 *)((int)piVar3 + 0x126) = *(undefined2 *)((int)unaff_EBX + 0x15a);
        *(short *)(piVar3 + 0x4a) = (short)unaff_EBX[0x57];
        *(char *)(piVar3 + 0x4b) = local_9;
        *(byte *)((int)piVar3 + 0x12e) = *(byte *)((int)unaff_EBX + 0x15e) >> 2 & 1;
        if (((short)piVar3[0x48] == 3) && ((*(byte *)((int)unaff_EBX + 0x15e) & 8) != 0)) {
          *(undefined1 *)((int)piVar3 + 0x12f) = 1;
          return 1;
        }
        *(undefined1 *)((int)piVar3 + 0x12f) = 0;
        return 1;
      }
      uVar2 = uVar2 + 1;
      piVar3 = piVar3 + 0x4c;
    } while ((int)uVar2 < 9);
  }
  return uVar2 & 0xffffff00;
}
#endif
```

## network_game_session_reset.c

```
// network_game_session_reset  (Ghidra: network_channel_table_initialize, already named --
// RENAMED here, see below)
// address 0x4de470, size 105 bytes
// name confidence: 0.6   rewrite confidence: 0.65
// evidence: every field this function touches matches types/networking.h's network_game_session
// and network_player_entry exactly: the first zero-loop is 0xec dwords == 0x3b0 bytes == the
// whole session; the byte at +0x19d (set to 0x10/16) is maximum_players ("initialized to 16" per
// the header); the word at +0x1a0 (zeroed) is player_count; the dword at +0xeb dwords == byte
// +0x3ac is unknown_3ac ("copied from 0x0071c2c1" per the header, and DAT_0071c2c1 is exactly
// what this function reads); and the 16-entry, stride-0x20 loop starting at byte +0x1bf writes
// -1/0xff into player[i].machine_index/machine_player_index/unknown_1e/slot_index (all
// documented "0xff/free when unused"), 0xffff into color_index/unknown_1a ("0xffff when
// unused"), and a zero word at player[i].name[0] -- i.e. every one of the 16 network_player_entry
// rows reset to its documented empty state.
// RENAMED: a previous naming pass called this network_channel_table_initialize, evidently
// reading the per-player reset loop as a distinct "channel key table"; the field-by-field match
// above shows it is squarely a network_game_session reset (chiefly the player table), so this
// rewrite renames it. Both names describe the same 16-entry, stride-0x20 loop.
// register convention: session in EDX (in_EDX). blam-cc: EDX -> session
```

```
#if 0
Original Ghidra decompilation (0x4de470):

void network_channel_table_initialize(void)

{
  undefined1 *puVar1;
  int iVar2;
  undefined4 *in_EDX;
  undefined4 *puVar3;
  bool bVar4;

  puVar3 = in_EDX;
  for (iVar2 = 0xec; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  puVar3 = in_EDX + 0x20;
  for (iVar2 = 0x21; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)(in_EDX + 0x68) = 0;
  puVar1 = (undefined1 *)((int)in_EDX + 0x1bf);
  iVar2 = 0x10;
  do {
    puVar1[-1] = 0xff;
    *puVar1 = 0xff;
    puVar1[1] = 0xff;
    puVar1[2] = 0xff;
    *(undefined2 *)(puVar1 + -0x1d) = 0;
    *(undefined2 *)(puVar1 + -5) = 0xffff;
    *(undefined2 *)(puVar1 + -3) = 0xffff;
    puVar1 = puVar1 + 0x20;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  bVar4 = DAT_0071c2c1 != '\0';
  *(undefined1 *)((int)in_EDX + 0x19d) = 0x10;
  *(bool *)(in_EDX + 0xeb) = bVar4;
  return;
}
#endif
```

## network_game_settings_ack_send.c

```
// network_game_settings_ack_send  (Ghidra: FUN_004d9f50; renamed, no prior name)
// address 0x4d9f50, size 455 bytes
// name confidence: 0.4   rewrite confidence: 0.25 (LOW -- same stack-frame caveat as
// network_game_settings_packet_send.c)
// evidence: out/phase4/networking_functions.md summary ("Builds and queues a small
// acknowledgement message while the local connection is in state 2 or 3 (used right after
// receiving the game-settings/map message); a no-op otherwise"); called from
// network_game_settings_packet_receive.c (0x4d9800, same task batch) the first time a
// game-settings packet is applied.
// register convention: `param_1` (client, byte-offset based here) and `param_2` (a template
// table row index) are both genuine cdecl stack parameters this time (Ghidra recovered them).
// // blam-cc: stack -> client, template_row
// UNSURE (major): same "Function: __chkstk replaced with injection: alloca_probe" stack-frame
// caveat as network_game_settings_packet_send.c applies here too. The named Ghidra locals are
// modeled as one shared byte buffer (`frame`), with each local's offset computed as
// 0x204e (local_204e's own suffix, the lowest-addressed named local) minus that local's suffix;
// this makes the later "copy 8 dwords starting at local_2048 into local_2028" and "local_2030 =
// local_1eee" statements consistent contiguous-memory operations instead of nonsensical
// small-to-small copies, which is the only reading that fits the byte math.
// UNSURE: `DAT_00712dd8` is the same template table referenced (without an index) in
// network_game_settings_packet_send.c; here it is indexed by `template_row * 0x801` dwords
// (0x801, one more than the 0x7ff-dword copy size, implying each row carries one extra trailing
// dword this function never reads).

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
Original Ghidra decompilation (0x4d9f50):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

char FUN_004d9f50(undefined1 *param_1,short param_2)

{
  int iVar1;
  ushort *puVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  wchar_t *pwVar7;
  undefined4 *puVar8;
  int iVar9;
  bool bVar10;
  bool bVar11;
  char local_204e;
  wchar_t local_2048 [11];
  undefined2 local_2032;
  undefined2 local_2030;
  undefined2 local_202e;
  undefined1 local_202c;
  undefined1 local_202b;
  undefined1 local_202a;
  undefined1 local_2029;
  undefined4 local_2028 [8];
  undefined4 local_2008;
  undefined2 local_1eee;
  undefined4 uStack_c;

  uStack_c = 0x4d9f60;
  puVar6 = &DAT_00712dd8 + param_2 * 0x801;
  puVar8 = &local_2008;
  for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar8 = *puVar6;
    puVar6 = puVar6 + 1;
    puVar8 = puVar8 + 1;
  }
  local_202b = (undefined1)param_2;
  local_202c = *param_1;
  _wcsncpy(local_2048,(wchar_t *)((int)&local_2008 + 2),0xb);
  local_2030 = local_1eee;
  local_202e = 0xffff;
  local_202a = 0xff;
  local_2029 = 0xff;
  local_2032 = 0;
  switch(*(undefined2 *)(param_1 + 0xeda)) {
  case 0:
  case 1:
  case 4:
    return '\0';
  case 2:
    pwVar7 = local_2048;
    puVar6 = local_2028;
    for (iVar3 = 8; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *(undefined4 *)pwVar7;
      pwVar7 = pwVar7 + 2;
      puVar6 = puVar6 + 1;
    }
    puVar2 = (ushort *)network_prepare_challenge_packet();
    if (puVar2 == (ushort *)0x0) {
      return '\x01';
    }
    iVar9 = *(int *)(param_1 + 0xadc);
    iVar5 = (uint)(*puVar2 >> 4) * 8;
    iVar3 = iVar5 + 1;
    if ((*(byte *)(iVar9 + 0xa8c) & 1) != 0) {
      return '\x01';
    }
    iVar4 = ((*(int *)(iVar9 + 0x24) + *(int *)(iVar9 + 0x1c) * -8) - *(int *)(iVar9 + 0x20)) + 1;
    bVar11 = SBORROW4(iVar3,iVar4);
    iVar1 = iVar3 - iVar4;
    bVar10 = iVar3 == iVar4;
    break;
  case 3:
    pwVar7 = local_2048;
    puVar6 = local_2028;
    for (iVar3 = 8; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *(undefined4 *)pwVar7;
      pwVar7 = pwVar7 + 2;
      puVar6 = puVar6 + 1;
    }
    puVar2 = (ushort *)network_prepare_challenge_packet();
    if (puVar2 == (ushort *)0x0) {
      return '\x01';
    }
    iVar9 = *(int *)(param_1 + 0xadc);
    iVar5 = (uint)(*puVar2 >> 4) * 8;
    iVar3 = iVar5 + 1;
    if ((*(byte *)(iVar9 + 0xa8c) & 1) != 0) {
      return '\x01';
    }
    iVar4 = ((*(int *)(iVar9 + 0x24) + *(int *)(iVar9 + 0x1c) * -8) - *(int *)(iVar9 + 0x20)) + 1;
    bVar11 = SBORROW4(iVar3,iVar4);
    iVar1 = iVar3 - iVar4;
    bVar10 = iVar3 == iVar4;
    break;
  default:
    return '\x01';
  }
  local_204e = '\x01';
  if ((bVar10 || bVar11 != iVar1 < 0) || (local_204e = FUN_004ddb60(iVar9,1), local_204e != '\0')) {
    *(int *)(iVar9 + 0xa80) = *(int *)(iVar9 + 0xa80) + iVar5 + 1;
    bit_stream_write_bits_chunked(1);
    *(undefined1 *)(iVar9 + 0x2c) = 0;
    bit_stream_write_bits_chunked(iVar5);
    *(undefined1 *)(iVar9 + 0x2c) = 0;
  }
  return local_204e;
}
#endif
```

## network_game_settings_broadcast_send.c

```
// network_game_settings_broadcast_send  (Ghidra: FUN_004df0e0; named per this rewrite)
// address 0x4df0e0, size 209 bytes
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Packages a 32-byte game-settings record
// together with the current tick, encodes and broadcasts it as message type 0x18, and records
// the send via network_object_record_last_sender." Follows the same data_packet_group_encode_packet /
// network_message_block_build / FUN_004e19c0 broadcast idiom as network_prepare_challenge_packet.c and
// network_game_server_host_dispose.c's challenge-packet send.
// UNSURE: data_packet_group_encode_packet's own full signature (src/memory) does not line up
// with the 4 arguments visible here either; kept as a local minimal prototype, matching the
// precedent in network_send_join_request_packet.c.
```

```
#if 0
Original Ghidra decompilation (0x4df0e0):

undefined4 FUN_004df0e0(undefined4 param_1,undefined4 *param_2)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 local_628;
  undefined4 local_624 [8];
  int local_604;

  puVar3 = local_624;
  for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = *param_2;
    param_2 = param_2 + 1;
    puVar3 = puVar3 + 1;
  }
  local_604 = *(int *)(DAT_006f1d6c + 0xc) + 0x21;
  local_628 = 0x600;
  cVar1 = data_packet_group_encode_packet(local_624,&local_628,0x18,1);
  if (cVar1 != '\0') {
    iVar2 = FUN_00440350(local_628);
    if (iVar2 != 0) {
      FUN_004e19c0(0,iVar2,1,0,0,3);
      FUN_004df900(param_1);
      return 1;
    }
  }
  return 0;
}
#endif
```

## network_game_start_new_server_from_profile.c

```
// network_game_start_new_server_from_profile  (Ghidra: already named)
// address 0x4e40f0, size 88 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md ("Prepares a default server-name/password
// profile on the stack ... and starts a new hosted game using it"); types/networking.h's
// network_client_begin_connect_scratch note explicitly documents this exact 0x7ff-dword
// memcpy as "overrun the array as the binary has it" -- the copy fills one contiguous 2047-dword
// (0x1ffc-byte) buffer whose tail 867*4..867*4+0x11f bytes become the name and password
// arguments network_game_start_new_server_with_name_and_password.c (this batch) actually reads;
// everything else copied is unused by this call path.
// register convention: __cdecl, one recognized parameter (param_1, forwarded unchanged).
// UNSURE: FUN_0053a150's exact role (foreign, builds a default profile directly into the
// destination buffer when no cached template exists yet); UNSURE: the split point between the
// "name" and "password" regions is inferred from network_game_start_new_server_with_name_and_password's
// own reads (wcsncpy of 0x3f chars, then of 8 chars) rather than independently confirmed here.
```

```
#if 0
Original Ghidra decompilation (0x4e40f0), from tools/pack.py 0x4e40f0:

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void network_game_start_new_server_from_profile(undefined4 param_1)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 local_2008 [867];
  undefined1 local_127c [288];
  undefined1 local_115c [4432];
  undefined4 uStack_c;

  uStack_c = 0x4e4100;
  if (DAT_00714dd4 == -1) {
    FUN_0053a150();
  }
  else {
    puVar2 = &DAT_00712dd8;
    puVar3 = local_2008;
    for (iVar1 = 0x7ff; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar3 = puVar3 + 1;
    }
  }
  network_game_start_new_server_with_name_and_password(param_1,local_127c,local_115c);
  return;
}
#endif
```

## network_game_start_new_server_with_name_and_password.c

```
// network_game_start_new_server_with_name_and_password  (Ghidra: already named)
// address 0x4e4150, size 504 bytes
// name confidence: 0.65   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md; types/networking.h network_server_globals
// (listen_channel/flags/password) and network_channel (listening at +0xae0). Disassembly
// (objdump -d -M intel) pins the wide-name destination precisely: `add eax,0x8` right before the
// first wcsncpy, with the forced NUL written at `[esi+0x86]` -- i.e. server+0x008..+0x086, which
// is the leading span of network_game_session (session+0x000..+0x07e) that types/networking.h
// currently records as message_callback/unknown_004/unknown_07e. This function writes a 63-wide-
// character name there directly, which the header's own account of that region (only ever
// zeroed by network_channel_table_initialize, not otherwise resolved) does not cover -- flagged
// UNSURE rather than reinterpreting the header's fields.
// register convention: __cdecl, three recognized parameters.
// UNSURE: unused is never read anywhere in this function's body; forwarded here only because
// network_game_start_new_server_from_profile.c (this batch) passes it through. UNSURE: the
// server+0x008..+0x086 wide-name write described above; also +0x9d5, which this function clears
// but which falls inside network_server_globals::unknown_9bc (no individual field name).
```

```
#if 0
Original Ghidra decompilation (0x4e4150), from tools/pack.py 0x4e4150:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4
network_game_start_new_server_with_name_and_password
          (undefined4 param_1,wchar_t *param_2,wchar_t *param_3)

{
  int *piVar1;
  char cVar2;
  int iVar3;
  bool bVar4;

  if (DAT_0071c2d4 != (int *)0x0) {
    network_game_server_host_dispose(DAT_0071c2d4);
    DAT_0071c2d4 = (int *)0x0;
    DAT_0071c2dd = 0;
  }
  FUN_004dde70();
  if (*param_2 == L'\0') {
    param_2 = L"Halo";
  }
  DAT_0069fe00 = DAT_006894a2;
  network_channels_open();
  if (DAT_006869be == '\0') goto LAB_004e4289;
  DAT_0069b350 = (uint)(DAT_006894a2 == '\x01');
  iVar3 = network_game_server_host_create();
  cVar2 = (char)iVar3;
  if (cVar2 == '\x01') {
    if ((*(byte *)((int)DAT_0071c2d4 + 6) >> 2 & 1) == 0) {
      DAT_0071c2d8 = network_session_create();
      cVar2 = '\0';
      if (DAT_0071c2d8 == 0) goto LAB_004e4285;
      DAT_0071c2de = 0;
      *(undefined4 *)(DAT_0071c2d8 + 0xf4c) = 4;
    }
    DAT_0068e680 = 0xffffffff;
    DAT_0068e684 = 0xffffffff;
    DAT_00718f8c = 0;
    DAT_00718f90 = 0;
    _DAT_006b2f28 = 0;
    _DAT_006b2f68 = 0;
    DAT_0068e688 = 0xffffffff;
    cVar2 = FUN_00463b20();
    if (cVar2 == '\0') {
LAB_004e4289:
      if (DAT_0071c2d4 != (int *)0x0) {
        network_game_server_host_dispose(DAT_0071c2d4);
        DAT_0071c2d4 = (int *)0x0;
        DAT_0071c2dd = 0;
      }
      FUN_004dde70();
      return 0;
    }
    DAT_00687b18 = 0xffffffff;
    game_engine_apply_current_custom_variant();
    FUN_0045fc80();
    DAT_00719720 = 2;
    FUN_004df640();
    DAT_0071c2dc = 1;
  }
  else {
LAB_004e4285:
    if (cVar2 == '\0') goto LAB_004e4289;
  }
  piVar1 = DAT_0071c2d4;
  if (DAT_0071c2d4 != (int *)0x0) {
    iVar3 = *DAT_0071c2d4;
    *(byte *)((int)DAT_0071c2d4 + 6) = *(byte *)((int)DAT_0071c2d4 + 6) | 1;
    *(undefined1 *)(iVar3 + 0xae0) = 1;
    *(undefined1 *)((int)piVar1 + 0x9d5) = 0;
    _wcsncpy((wchar_t *)(piVar1 + 2),param_2,0x3f);
    *(undefined2 *)((int)piVar1 + 0x86) = 0;
    _wcsncpy((wchar_t *)(piVar1 + 0x27f),param_3,8);
    iVar3 = DAT_00699584;
    bVar4 = DAT_00699584 < 0;
    *(undefined2 *)(piVar1 + 0x283) = 0;
    if (bVar4) {
      iVar3 = 0;
      DAT_00699584 = iVar3;
    }
    else if (0x10 < iVar3) {
      iVar3 = 0x10;
      DAT_00699584 = iVar3;
    }
    *(char *)((int)piVar1 + 0x1a5) = (char)iVar3;
    widget_close_all();
    *(undefined1 *)((int)piVar1 + 0x9fa) = 1;
    if ((*(byte *)((int)piVar1 + 6) >> 2 & 1) == 0) {
      DAT_00718f8c = 2;
    }
    return 1;
  }
  FUN_004dde70();
  return 0;
}
#endif
```

## network_map_cycle_list_broadcast.c

```
// network_map_cycle_list_broadcast  (Ghidra: FUN_004deec0; named per this rewrite)
// address 0x4deec0, size 190 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Builds and broadcasts a message (type 0x35)
// listing every entry returned by data_iterator_next, e.g. the map/variant cycle list, via
// FUN_004ec940/FUN_004e19c0." Up to 16 entries, each an 8-byte (byte, dword) record staged into
// the static scratch array at 0x00861d60.
// UNSURE: data_iterator_next's own `data_iterator *iterator` argument is elided at both call
// sites; a NULL/zeroed local is used here so the iterator can advance from a fixed starting
// point, but the real source iterator (presumably a tag iteration over the map/variant list)
// could not be identified.
// UNSURE: the original loops unconditionally while the iterator keeps returning items, writing
// into the static global scratch array at 0x00861d60 (which may have had headroom beyond the 16
// entries `local_440` can index); this rewrite uses local stack arrays instead and adds a
// `count < 16` bound to avoid overflowing them, which is a defensive deviation from the literal
// decompile rather than an observed limit.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original
```

```
#if 0
Original Ghidra decompilation (0x4deec0):

void FUN_004deec0(void)

{
  int iVar1;
  int count;
  undefined1 *puVar2;
  void **ppvVar3;
  void *local_440 [16];
  undefined1 local_400 [1024];

  ppvVar3 = local_440;
  for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
    *ppvVar3 = (void *)0x0;
    ppvVar3 = ppvVar3 + 1;
  }
  count = 0;
  iVar1 = data_iterator_next();
  if (iVar1 != 0) {
    puVar2 = &DAT_00861d60;
    do {
      *puVar2 = *(undefined1 *)(iVar1 + 0x67);
      *(undefined4 *)(puVar2 + 4) = *(undefined4 *)(iVar1 + 0xdc);
      local_440[count] = puVar2;
      count = count + 1;
      puVar2 = puVar2 + 8;
      iVar1 = data_iterator_next();
    } while (iVar1 != 0);
    if (0 < count) {
      message_delta_encode_message(0,0x35,0,local_440,0,count,'\0');
      FUN_004e19c0(1,local_400,0,0,0,3);
    }
  }
  return;
}
#endif
```

## network_object_owner_team_index_desired.c

```
// network_object_owner_team_index_desired  (Ghidra: FUN_004e0cf0, unnamed)
// address 0x4e0cf0, size 63 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Looks up the player datum referenced by the
// object's stored index and returns a byte field (likely team or slot id) from it, or -1 if
// unavailable." `object+0xc` is types/objects.h's object::unknown_00c, here read as a plain
// uint16 index (not a validated datum_index) into machine_to_player; the returned byte at
// player+0x67 matches types/game.h player::team_index_desired exactly.
// register convention: EAX = object (object *).
// blam-cc: EAX -> object
// UNSURE: this reveals object::unknown_00c is (at least sometimes) used as a raw
// machine_to_player index rather than a general datum_index; not renamed in types/objects.h.
// UNSURE: `&DAT_006b1460 + index != NULL` in the original is a tautology (the table's address
// plus a small index is never NULL); preserved as a literal check.
```

```
#if 0
Original Ghidra decompilation (0x4e0cf0):

int FUN_004e0cf0(void)

{
  int in_EAX;
  int iVar1;

  if (((*(ushort *)(in_EAX + 0xc) != 0xffff) &&
      (&DAT_006b1460 + *(ushort *)(in_EAX + 0xc) != (int *)0x0)) &&
     ((&DAT_006b1460)[*(ushort *)(in_EAX + 0xc)] != -1)) {
    iVar1 = datum_get();
    if (iVar1 != 0) {
      return (int)*(char *)(iVar1 + 0x67);
    }
  }
  return -1;
}
#endif
```

## network_object_release_ownership_claim.c

```
// network_object_release_ownership_claim  (Ghidra: FUN_004dfc10, unnamed)
// address 0x4dfc10, size 119 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Releases the network ownership claim on the
// object resolved from an unaff_BL machine/index value, notifying FUN_004779d0 and clearing
// the local ownership state via FUN_00466e80/FUN_00466ee0." Shares the exact
// player_data (0x0087a480)-lookup idiom with network_game_server_handoff_object_ownership.c
// (team at player+0x20, salt check against the resolved datum's high half).
// register convention: BL = slot_index (uint8_t).
// blam-cc: BL -> slot_index
// UNSURE: player_data_iterator_advance's, FUN_004779d0's, and the two ownership-state
// helpers' real signatures are inferred only from this and the sibling call site in
// network_game_server_handoff_object_ownership.c.
```

```
#if 0
Original Ghidra decompilation (0x4dfc10):

void FUN_004dfc10(void)

{
  int iVar1;
  int iVar2;
  short sVar3;
  undefined1 unaff_BL;
  short sVar4;

  iVar1 = FUN_004d98f0(unaff_BL);
  if (((iVar1 != -1) && (sVar4 = (short)iVar1, -1 < sVar4)) &&
     (sVar4 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar2 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar4;
    sVar4 = *(short *)(iVar2 + *(int *)(DAT_0087a480 + 0x34));
    if ((sVar4 != 0) && ((sVar3 = (short)((uint)iVar1 >> 0x10), sVar3 == 0 || (sVar4 == sVar3)))) {
      FUN_004779d0(*(undefined4 *)(iVar2 + *(int *)(DAT_0087a480 + 0x34) + 0x20));
      iVar1 = FUN_00466e80();
      if (iVar1 != -1) {
        FUN_00466ee0(0);
      }
    }
  }
  return;
}
#endif
```

## network_player_assign_random_color.c

```
// network_player_assign_random_color  (Ghidra: FUN_004df790; named per this rewrite)
// address 0x4df790, size 165 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: types/networking.h cites this address directly: "network_player_entry (... 0x4df790
// colour assignment)". out/phase4/networking_functions.md: "Randomly assigns a player colour
// index that isn't already in use by another active player, widening the candidate range after
// 10 failed attempts, and stores it at param_1+0x18." param_1+0x18 matches
// network_player_entry.color_index; the scanned array (container+0x1c2 == session.players[0]+
// 0x20 == session.players[1].color_index, stride 0x10 shorts) matches every active player's own
// color_index field.
// register convention: session container in EAX (in_EAX). blam-cc: EAX -> session, stack ->
// entry
```

```
#if 0
Original Ghidra decompilation (0x4df790):

void FUN_004df790(int param_1)

{
  bool bVar1;
  char cVar2;
  int in_EAX;
  short sVar3;
  uint uVar4;
  short *psVar5;
  int iVar6;
  int local_c;

  local_c = 0;
  uVar4 = DAT_00719cd4;
  do {
    uVar4 = uVar4 * 0x19660d + 0x3c6ef35f;
    if (local_c < 10) {
      sVar3 = (short)((uVar4 >> 0x10) * 3 >> 0x10);
    }
    else {
      sVar3 = (short)((uVar4 >> 0x10) * 0x11 >> 0x10);
    }
    bVar1 = true;
    iVar6 = 0;
    psVar5 = (short *)(in_EAX + 0x1c2);
    DAT_00719cd4 = uVar4;
    do {
      cVar2 = FUN_004de9f0();
      if ((cVar2 != '\0') && (*psVar5 == sVar3)) {
        bVar1 = false;
        break;
      }
      iVar6 = iVar6 + 1;
      psVar5 = psVar5 + 0x10;
    } while (iVar6 < 0x10);
    local_c = local_c + 1;
    if (bVar1) {
      *(short *)(param_1 + 0x18) = sVar3;
      return;
    }
  } while( true );
}
#endif
```

## network_player_entry_add.c

```
// network_player_entry_add  (Ghidra: FUN_004de4e0; named per this rewrite)
// address 0x4de4e0, size 259 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: types/networking.h's own comment on network_player_entry cites this address
// directly: "network_player_entry (0x4de4e0 add, 0x4de5f0 update, 0x4de640 remove, 0x4de900
// find, 0x4de9f0 validate, 0x4df790 colour assignment)". Every offset (session.player_count at
// +0x1a0, session.maximum_players at +0x19d, players[] at +0x1a2 stride 0x20 with
// machine_index/machine_player_index at +0x1c/+0x1d and slot_index at +0x1f) matches
// types/networking.h's network_game_session and network_player_entry exactly. `in_EAX + 7`
// (in_EAX is `undefined4 *`, so this scales to byte +0x1c) and `(int)in_EAX + 0x1d` are the
// incoming record's own machine_index/machine_player_index, validated to be in 0..15 / ==0
// (matching the header's "machine_player_index always 0 on PC build" note) before the lookup.
// The original's unrolled 4-way duplicate-key scan (slots i..i+3 per pass) only distinguishes
// "some slot matches" from "none matches", so the single per-slot loop below is equivalent.
// register convention: session in param_1 (stack), incoming record in EAX (in_EAX). blam-cc:
// EAX -> incoming, stack -> session

// VERIFIED against disassembly 0x4de4e0..0x4de5e3 (2026-09-30): range checks, unrolled duplicate scan (== plain per-slot scan), validate(EAX), free-row search on slot_index==-1, preferred incoming slot, 8-dword copy, player_count++; only AL (1) is returned
```

```
#if 0
Original Ghidra decompilation (0x4de4e0):

uint FUN_004de4e0(int param_1)

{
  char cVar1;
  char cVar2;
  undefined4 *in_EAX;
  uint uVar3;
  char *pcVar4;
  int iVar5;
  char *pcVar6;
  char *pcVar7;
  undefined4 *puVar8;

  uVar3 = (uint)in_EAX & 0xffffff00;
  if ((((*(short *)(param_1 + 0x1a0) < (short)*(char *)(param_1 + 0x19d)) &&
       (cVar1 = *(char *)(in_EAX + 7), -1 < cVar1)) && (cVar1 < '\x10')) &&
     ((cVar2 = *(char *)((int)in_EAX + 0x1d), -1 < cVar2 && (cVar2 < '\x01')))) {
    iVar5 = 0;
    pcVar4 = (char *)(param_1 + 0x1bf);
    do {
      if ((pcVar4[-1] == cVar1) && (*pcVar4 == cVar2)) break;
      if ((pcVar4[0x1f] == cVar1) && (pcVar4[0x20] == cVar2)) {
        iVar5 = iVar5 + 1;
        break;
      }
      if ((pcVar4[0x3f] == cVar1) && (pcVar4[0x40] == cVar2)) {
        iVar5 = iVar5 + 2;
        break;
      }
      if ((pcVar4[0x5f] == cVar1) && (pcVar4[0x60] == cVar2)) {
        iVar5 = iVar5 + 3;
        break;
      }
      iVar5 = iVar5 + 4;
      pcVar4 = pcVar4 + 0x80;
    } while (iVar5 < 0x10);
    if ((iVar5 == 0x10) && (pcVar4 = (char *)network_player_entry_validate(), (char)pcVar4 != '\0')) {
      pcVar6 = (char *)0x0;
      pcVar7 = (char *)(param_1 + 0x1c1);
      do {
        pcVar4 = pcVar6;
        if (*pcVar7 == -1) break;
        pcVar6 = pcVar6 + 1;
        pcVar7 = pcVar7 + 0x20;
        pcVar4 = (char *)0xffffffff;
      } while ((int)pcVar6 < 0x10);
      cVar1 = *(char *)((int)in_EAX + 0x1f);
      if ((cVar1 != -1) && (pcVar4 != (char *)(int)cVar1)) {
        pcVar4 = (char *)(int)cVar1;
      }
      if (pcVar4 != (char *)0xffffffff) {
        *(char *)((int)in_EAX + 0x1f) = (char)pcVar4;
        pcVar7 = (char *)((int)pcVar4 * 0x20 + 0x1a2 + param_1);
        for (iVar5 = 8; iVar5 != 0; iVar5 = iVar5 + -1) {
          *pcVar7 = *puVar8;
        }
        *(short *)(param_1 + 0x1a0) = *(short *)(param_1 + 0x1a0) + 1;
        return CONCAT31((int3)((uint)((int)pcVar4 * 0x20) >> 8),1);
      }
    }
  }
  return 0;
}
#endif
```

## network_player_entry_find.c

```
// network_player_entry_find  (Ghidra: FUN_004de900; named per this rewrite)
// address 0x4de900, size 65 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/networking.h cites this address directly under network_player_entry: "0x4de900
// find". Validates the key record via network_player_entry_validate, then scans session->players[] for a
// machine_index/machine_player_index match. Both callers in this batch (network_player_entry_
// update.c, network_player_entry_remove.c) only ever test the low byte of this function's
// return (truthy/falsy); only AL is defined in the original, so a plain 0/1 return is equivalent.
// register convention: session in param_1 (stack), key in ESI (unaff_ESI). blam-cc: ESI -> key,
// stack -> session

// VERIFIED against disassembly 0x4de900..0x4de941 (2026-09-30): validate(ESI key via EAX), scan of machine_index (+0x1c) then machine_player_index (+0x1d) over 16 rows
```

```
#if 0
Original Ghidra decompilation (0x4de900):

uint FUN_004de900(int param_1)

{
  uint uVar1;
  char *pcVar2;
  int iVar3;
  int unaff_ESI;

  uVar1 = network_player_entry_validate();
  if ((char)uVar1 == '\0') {
    return uVar1 & 0xffffff00;
  }
  iVar3 = 0;
  pcVar2 = (char *)(param_1 + 0x1bf);
  while ((pcVar2[-1] != *(char *)(unaff_ESI + 0x1c) || (*pcVar2 != *(char *)(unaff_ESI + 0x1d)))) {
    iVar3 = iVar3 + 1;
    pcVar2 = pcVar2 + 0x20;
    if (0xf < iVar3) {
      return (uint)pcVar2 & 0xffffff00;
    }
  }
  return CONCAT31((int3)((uint)pcVar2 >> 8),1);
}
#endif
```

## network_player_entry_remove.c

```
// network_player_entry_remove  (Ghidra: FUN_004de640; named per this rewrite)
// address 0x4de640, size 130 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/networking.h cites this address directly under network_player_entry: "0x4de640
// remove". After the network_player_entry_find pre-check, re-scans for the same
// (machine_index, machine_player_index) key and resets that row to its documented empty state
// (matching network_game_session_reset.c's field-by-field evidence), decrementing player_count.
// register convention: session in EBX (unaff_EBX), key in EAX (in_EAX). blam-cc: EAX -> key,
// EBX -> session
```

```
#if 0
Original Ghidra decompilation (0x4de640):

uint FUN_004de640(void)

{
  undefined2 *puVar1;
  char extraout_AL;
  int in_EAX;
  uint uVar2;
  uint3 extraout_var;
  int unaff_EBX;
  char *pcVar3;
  int iVar4;

  uVar2 = FUN_004de900();
  if ((char)uVar2 == '\0') {
    return uVar2 & 0xffffff00;
  }
  iVar4 = 0;
  pcVar3 = (char *)(unaff_EBX + 0x1be);
  while( true ) {
    network_player_entry_validate();
    if (((extraout_AL != '\0') && (*pcVar3 == *(char *)(in_EAX + 0x1c))) &&
       (pcVar3[1] == *(char *)(in_EAX + 0x1d))) break;
    iVar4 = iVar4 + 1;
    pcVar3 = pcVar3 + 0x20;
    if (0xf < iVar4) {
      return (uint)extraout_var << 8;
    }
  }
  puVar1 = (undefined2 *)(iVar4 * 0x20 + 0x1a2 + unaff_EBX);
  *(undefined1 *)(puVar1 + 0xe) = 0xff;
  *(undefined1 *)((int)puVar1 + 0x1d) = 0xff;
  *(undefined1 *)((int)puVar1 + 0x1f) = 0xff;
  *(undefined1 *)((int)puVar1 + 0x1f) = 0xff;
  *puVar1 = 0;
  puVar1[0xc] = 0xffff;
  puVar1[0xd] = 0xffff;
  *(short *)(unaff_EBX + 0x1a0) = *(short *)(unaff_EBX + 0x1a0) + -1;
  return CONCAT31((int3)((uint)puVar1 >> 8),1);
}
#endif
```

## network_player_entry_update.c

```
// network_player_entry_update  (Ghidra: FUN_004de5f0; named per this rewrite)
// address 0x4de5f0, size 75 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: types/networking.h cites this address directly under network_player_entry: "0x4de5f0
// update". Finds the row via network_player_entry_find (0x4de900) then overwrites it with the
// incoming 32-byte record from `in_EAX`, after confirming the found slot's own
// machine_player_index/color_index still match (a stale-slot guard).
// UNSURE: `*(char*)(puVar3+7) == *(char*)(in_EAX+7)` compares byte +0x1c of the FOUND slot
// (puVar3 is `undefined4*`, so +7 scales to +0x1c = machine_index) against the incoming record's
// own +0x1c -- i.e. this re-checks machine_index after already finding by it, which is
// redundant; kept literally.
// register convention: session in ECX (in_ECX), incoming record in EAX (in_EAX). blam-cc:
// EAX -> incoming, ECX -> session
```

```
#if 0
Original Ghidra decompilation (0x4de5f0):

uint FUN_004de5f0(void)

{
  undefined4 *in_EAX;
  uint uVar1;
  int in_ECX;
  int iVar2;
  undefined4 *puVar3;

  uVar1 = FUN_004de900();
  if ((char)uVar1 != '\0') {
    uVar1 = *(char *)((int)in_EAX + 0x1f) * 0x20;
    puVar3 = (undefined4 *)(uVar1 + 0x1a2 + in_ECX);
    if ((*(char *)(uVar1 + 0x1bf + in_ECX) == *(char *)((int)in_EAX + 0x1d)) &&
       (*(char *)(puVar3 + 7) == *(char *)(in_EAX + 7))) {
      for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar3 = *in_EAX;
        in_EAX = in_EAX + 1;
        puVar3 = puVar3 + 1;
      }
      return CONCAT31((int3)(uVar1 >> 8),1);
    }
  }
  return uVar1 & 0xffffff00;
}
#endif
```

## network_player_entry_validate.c

```
// network_player_entry_validate  (Ghidra: FUN_004de9f0; named per this rewrite)
// address 0x4de9f0, size 138 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Register-based (EAX=player-slot pointer)
// validity check: confirms the slot's type/index fields are in range and its embedded
// wide-character name is properly NUL-terminated within its fixed-size field." entry+0x1d
// (machine_player_index, checked 0..0) and entry[0xe] as a short index (byte +0x1c,
// machine_index, checked 0..15) match types/networking.h's network_player_entry; the name scan
// walks entry->name[0..11] (0x00..0x17) looking for a NUL, matching name[12].
// register convention: entry in EAX (in_EAX). blam-cc: EAX -> entry
```

```
#if 0
Original Ghidra decompilation (0x4de9f0):

uint FUN_004de9f0(void)

{
  short sVar1;
  short *in_EAX;
  short *psVar2;
  uint uVar3;

  if ((((in_EAX != (short *)0x0) && (-1 < *(char *)((int)in_EAX + 0x1d))) &&
      (*(char *)((int)in_EAX + 0x1d) < '\x01')) &&
     ((-1 < (char)in_EAX[0xe] && ((char)in_EAX[0xe] < '\x10')))) {
    uVar3 = 0;
    psVar2 = in_EAX;
    do {
      in_EAX = psVar2 + 1;
      if (*psVar2 == 0) {
LAB_004dea72:
        if (uVar3 < 0xc) {
          return CONCAT31((int3)((uint)in_EAX >> 8),1);
        }
        break;
      }
      sVar1 = *in_EAX;
      in_EAX = psVar2 + 2;
      if (sVar1 == 0) {
        uVar3 = uVar3 + 1;
        goto LAB_004dea72;
      }
      sVar1 = *in_EAX;
      in_EAX = psVar2 + 3;
      if (sVar1 == 0) {
        uVar3 = uVar3 + 2;
        goto LAB_004dea72;
      }
      sVar1 = *in_EAX;
      in_EAX = psVar2 + 4;
      if (sVar1 == 0) {
        uVar3 = uVar3 + 3;
        goto LAB_004dea72;
      }
      sVar1 = *in_EAX;
      in_EAX = psVar2 + 5;
      if (sVar1 == 0) {
        uVar3 = uVar3 + 4;
        goto LAB_004dea72;
      }
      sVar1 = *in_EAX;
      in_EAX = psVar2 + 6;
      if (sVar1 == 0) {
        uVar3 = uVar3 + 5;
        goto LAB_004dea72;
      }
      uVar3 = uVar3 + 6;
      psVar2 = in_EAX;
    } while (uVar3 < 0xc);
  }
  return (uint)in_EAX & 0xffffff00;
}
#endif
```

## network_player_name_collision_check.c

```
// network_player_name_collision_check  (Ghidra: FUN_004df6f0; named per this rewrite)
// address 0x4df6f0, size 60 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Checks whether the wide-character name pointed
// to by unaff_EBX already matches any active player's name in the 16-slot table, returning 0 on
// a collision." The scan base (container + 0x1aa) is exactly container->session.players[0].name
// when container is a network_client_globals/network_server_globals (session embedded at +8,
// players[] at session+0x1a2, name the first field of each 0x20-byte entry) -- 0x008 + 0x1a2 =
// 0x1aa.
// register convention: container in EAX (in_EAX), candidate name in EBX (unaff_EBX). blam-cc:
// EAX -> session, EBX -> candidate_name
```

```
#if 0
Original Ghidra decompilation (0x4df6f0):

undefined4 FUN_004df6f0(void)

{
  char cVar1;
  int in_EAX;
  int iVar2;
  wchar_t *unaff_EBX;
  wchar_t *_Str1;
  int iVar3;

  iVar3 = 0;
  _Str1 = (wchar_t *)(in_EAX + 0x1aa);
  do {
    cVar1 = FUN_004de9f0();
    if (cVar1 != '\0') {
      iVar2 = _wcscmp(_Str1,unaff_EBX);
      if (iVar2 == 0) {
        return 0;
      }
    }
    iVar3 = iVar3 + 1;
    _Str1 = _Str1 + 0x10;
  } while (iVar3 < 0x10);
  return 1;
}
#endif
```

## network_player_ping_field_update_and_report.c

```
// network_player_ping_field_update_and_report  (Ghidra: FUN_004dbaa0; renamed, no prior name)
// address 0x4dbaa0, size 342 bytes
// name confidence: 0.35   rewrite confidence: 0.25 (LOW -- several elided-register inputs; see
// UNSURE notes)
// evidence: out/phase4/networking_functions.md summary ("Updates a player-table ping/latency-
// related field, scans an iterator of entries to compute an elapsed-time delta, and queues a
// short reliable status message reporting it"). player_data (0x0087a480) offsets +0x20/+0x22/
// +0x34 match types/memory.h's data_array exactly; player+0x02/+0x67/+0xdc match types/game.h's
// player::local_player_index/team_index_desired/unknown_dc. Ghidra's own decompile carries a
// "Removing unreachable block" warning.
// register convention: none recovered by Ghidra; every value this function reads before its
// first real call is either a global or an elided register output. // blam-cc: none
// UNSURE (major): `FUN_004ec590` is called with no visible arguments and its result is read back
// through two locals (`local_58`, a player-table index; `local_54`, a value to store into that
// player's unknown_dc) that Ghidra shows as uninitialized until that call -- the classic
// register-output-not-tracked pattern used throughout this codebase. Modeled as two extra local
// variables assigned from that call's own (unrecoverable) implicit outputs; since no value can
// be recovered, they are left uninitialized here exactly as Ghidra shows, which the compiler is
// asked to accept via explicit (if arbitrary) initialization to 0/0xffffffff matching Ghidra's
// own sentinel conventions elsewhere in this cluster, not a confirmed value.
// UNSURE: the data_iterator (`local_44`/`local_4c`/`local_48` in Ghidra) is reconstructed as a
// real `data_iterator` bound to `player_data`; the "data ^ 0x69746572" dword is its +0x0c
// signature (types/memory.h).
// UNSURE: `network_server+0x9c0` falls inside types/networking.h's still-unresolved
// network_server_globals::unknown_9bc[0x3c] block; accessed via a raw offset rather than a new
// named field.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original
```

```
#if 0
Original Ghidra decompilation (0x4dbaa0):

/* WARNING: Removing unreachable block (ram,0x004dbaf2) */

void FUN_004dbaa0(void)

{
  int iVar1;
  char cVar2;
  short *psVar3;
  int iVar4;
  int iVar5;
  char local_62 [2];
  char *local_60;
  undefined4 local_5c;
  byte local_58;
  undefined4 local_54;
  uint local_50;
  undefined2 local_4c;
  undefined4 local_48;
  uint local_44;
  undefined1 local_40 [64];

  cVar2 = FUN_004ec590();
  local_50 = DAT_0087a480;
  if (cVar2 == '\x01') {
    cVar2 = -1;
    if (((local_58 != 0xffffffff) && ((short)(ushort)local_58 < *(short *)(DAT_0087a480 + 0x20))) &&
       (psVar3 = (short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)(short)(ushort)local_58 +
                          *(int *)(DAT_0087a480 + 0x34)), *psVar3 != 0)) {
      *(undefined4 *)(psVar3 + 0x6e) = local_54;
    }
    local_44 = local_50 ^ 0x69746572;
    local_4c = 0;
    local_48 = 0xffffffff;
    iVar4 = data_iterator_next();
    if (iVar4 != 0) {
      cVar2 = -1;
      do {
        if (*(short *)(iVar4 + 2) != -1) {
          cVar2 = *(char *)(iVar4 + 0x67);
          if ((cVar2 != -1) && (DAT_00719720 == 2)) {
            iVar1 = *(int *)(DAT_0071c2d4 + 0x9c0);
            iVar5 = FUN_00449210();
            *(int *)(iVar4 + 0xdc) = iVar5 - iVar1;
            return;
          }
          break;
        }
        iVar4 = data_iterator_next();
      } while (iVar4 != 0);
    }
    local_60 = local_62;
    local_5c = 0;
    local_62[0] = cVar2;
    iVar4 = message_delta_encode_message(0,0x34,0,&local_60,0,1,'\0');
    if (0 < iVar4) {
      local_62[1] = 1;
      if ((*(byte *)(*(int *)(DAT_0071c2d8 + 0xadc) + 0xa8c) & 1) == 0) {
        FUN_004dcdb0(*(int *)(DAT_0071c2d8 + 0xadc),local_40,local_62 + 1,0);
      }
    }
  }
  return;
}
#endif
```

## network_player_update_history_log_write.c

```
// network_player_update_history_log_write  (Ghidra: network_player_update_history_log_write,
// already named)
// address 0x4e7f90, size 95 bytes
// name confidence: 0.75   rewrite confidence: 0.8
// evidence: out/phase4/networking_functions.md; literal string "ServerPlayerUpdateHistory.log";
// player_update_history_log_write.c (this batch's sibling client-side logger, same fopen wrapper
// and mode string).
// register convention: none; __cdecl, format plus varargs on the stack.
// UNSURE: DAT_00710320's exact meaning beyond "server player-update-history logging enabled".
```

```
#if 0
Original Ghidra decompilation (0x4e7f90), from tools/pack.py 0x4e7f90:

void __cdecl network_player_update_history_log_write(char *format,...)

{
  FILE *_File;
  char local_400 [1024];

  _vsprintf(local_400,format,&stack0x00000008);
  if ((DAT_00710320 == '\x01') &&
     (_File = (FILE *)FUN_00624186(PTR_s_ServerPlayerUpdateHistory_log_0069a2c8,&DAT_0066b87c),
     _File != (FILE *)0x0)) {
    _fprintf(_File,local_400);
    _fclose(_File);
  }
  return;
}
#endif
```

## network_session_disconnect_with_error.c

```
// network_session_disconnect_with_error  (Ghidra: FUN_004d97e0; renamed, no prior name)
// address 0x4d97e0, size 31 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Records an error code and triggers a
// network-session disconnect/cleanup").
// register convention: the error code arrives in AX (in_AX). // blam-cc: AX -> error_code
// Stores error_code + 0x2b (an error-string index) as the pending join error only if none is pending.

// VERIFIED against disassembly 0x4d97e0..0x4d97ff (2026-09-30): word compare with -1, word store of error_code + 0x2b, flag store, tail call to chat_close
```

```
#if 0
Original Ghidra decompilation (0x4d97e0):

void FUN_004d97e0(void)

{
  short in_AX;

  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = in_AX + 0x2b;
  }
  DAT_0071c2de = 1;
  chat_close();
  return;
}
#endif
```

## network_session_host_cd_key_callback.c

```
// network_session_host_cd_key_callback  (not a Ghidra function; no C existed)
// address 0x5760a0, size 92 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x5760a0..0x5760fb: the gcd_authenticate_user callback (game id, local id,
//   authenticated, message, instance): a rejected key sends reason 4 to the machine with that CD key local id (+0x5c
//   of the 0x60 byte machines at server +0x3b8; NULL when none).
// blam-cc: cdecl (a gcdkey callback)
```

## network_session_host_dispatch_message.c

```
// network_session_host_dispatch_message  (Ghidra: FUN_00577e40; the qr2 player key callback)
// address 0x577e40, size 241 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x577e40..0x577f30: the qr2 player key callback (key, index, buffer, user
//   data); the earlier version modeled three arguments. The player at that active index (validated: index in range,
//   live, salt 0 or matching) has key 0x15 its name (at most 0x40 characters, ASCII) and 0x19 its team; the game
//   engine's +0xa0 hook answers other keys; anything unanswered is empty. (Name kept.)
// blam-cc: cdecl (a qr2 player key callback)
```

```
#if 0
Original Ghidra decompilation (0x577e40):

void FUN_00577e40(int param_1,undefined4 param_2,undefined4 param_3)

{
  char cVar1;
  short sVar2;
  int iVar3;
  undefined4 uVar4;
  short *psVar5;
  short sVar6;
  undefined4 *puVar7;
  undefined4 local_3f;

  iVar3 = FUN_0045c6f0();
  if (((iVar3 != -1) && (sVar2 = (short)iVar3, -1 < sVar2)) &&
     (sVar2 < *(short *)(DAT_0087a480 + 0x20))) {
    psVar5 = (short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar2 +
                      *(int *)(DAT_0087a480 + 0x34));
    sVar2 = *psVar5;
    if ((sVar2 != 0) && ((sVar6 = (short)((uint)iVar3 >> 0x10), sVar6 == 0 || (sVar2 == sVar6)))) {
      if (param_1 == 0x15) {
        puVar7 = &local_3f;
        for (iVar3 = 0xf; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar7 = 0;
          puVar7 = puVar7 + 1;
        }
        *(undefined2 *)puVar7 = 0;
        *(undefined1 *)((int)puVar7 + 2) = 0;
        uVar4 = FUN_00557950(0x40);
        FUN_00615590(param_3,uVar4);
        return;
      }
      if (param_1 == 0x19) {
        FUN_00616640(param_3,*(undefined4 *)(psVar5 + 0x10));
        return;
      }
      if (((DAT_006f1d20 != 0) && (*(code **)(DAT_006f1d20 + 0xa0) != (code *)0x0)) &&
         (cVar1 = (**(code **)(DAT_006f1d20 + 0xa0))(param_1,param_2,param_3), cVar1 != '\0')) {
        return;
      }
    }
  }
  FUN_00615590(param_3,&DAT_0065512c);
  return;
}
#endif
```

## network_session_host_dispose.c

```
// network_session_host_dispose  (Ghidra: FUN_005778f0; named per this rewrite)
// address 0x5778f0, size 74 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary: "Tears down the network channel/session
// object created by network_session_host_start, if one exists."
// register convention: no register-passed arguments.
// UNSURE: gcd_shutdown/qr2_shutdown are foreign (GameSpy-shaped) calls whose argument lists
// Ghidra elided entirely.
```

```
#if 0
Original Ghidra decompilation (0x5778f0):

void FUN_005778f0(void)

{
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

## network_session_host_natneg_callback.c

```
// network_session_host_natneg_callback  (not a Ghidra function; no C existed)
// address 0x578160, size 40 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x578160..0x578187: the query/report NAT negotiation callback (cookie): starts
//   NNBeginNegotiationWithSocket on the game socket's SOCKET (read through 0x6175f0, folded with ArrayLength) with
//   that cookie, client index 0, function_do_nothing as the progress callback and
//   network_session_host_natneg_completed.
// blam-cc: cdecl (the qr2 natneg callback)
```

## network_session_host_natneg_completed.c

```
// network_session_host_natneg_completed  (not a Ghidra function; no C existed)
// address 0x578120, size 49 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x578120..0x578150: the NAT negotiation completion callback (result, socket,
//   remote sockaddr_in, user data): on success it formats the remote address into a stack buffer with
//   gt2AddressToString and does nothing with it (a leftover).
// blam-cc: cdecl (a NAT negotiation callback)
```

## network_session_host_qr2_add_error.c

```
// network_session_host_qr2_add_error  (not a Ghidra function; no C existed -- the host's qr2 callback, which network_session_host_start passed as NULL)
// address 0x578100, size 24 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x578100..0x578117: the qr2 add-error callback (error, message, user data):
//   "qr2_adderror_callback - %s" to the console in its standard color.
// blam-cc: cdecl (a qr2 add-error callback)
```

## network_session_host_qr2_count.c

```
// network_session_host_qr2_count  (not a Ghidra function; no C existed -- the host's qr2 callback, which network_session_host_start passed as NULL)
// address 0x5780c0, size 88 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x5780c0..0x578117: the player / team count callback (key type, user data): 0
//   outside a game; the game engine's +0xa8 hook when it has one; otherwise players -> the active player count, teams
//   -> 2 with teams (else 0), anything else 0.
// blam-cc: cdecl (a qr2 count callback)
```

## network_session_host_qr2_key_list.c

```
// network_session_host_qr2_key_list  (not a Ghidra function; no C existed -- the host's qr2 callback, which network_session_host_start passed as NULL)
// address 0x577fb0, size 261 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x577fb0..0x5780b4: the key list callback (key type, key buffer, user data):
//   server keys 1 3 4 10 19 5 0x33 11 and, in a game, 0x36 8 6 12 7 13 0x34 0x35; in a game, player keys 0x15 0x16
//   0x18 0x19 and team keys 0x1c 0x1d.
// blam-cc: cdecl (a qr2 key list callback)
```

## network_session_host_qr2_server_key.c

```
// network_session_host_qr2_server_key  (not a Ghidra function; no C existed -- the host's qr2 callback, which network_session_host_start passed as NULL)
// address 0x5779c0, size 1116 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x5779c0..0x577e1b: the server key callback (key, buffer, user data). The game
//   engine's +0x9c hook may answer first. Without a server the value is empty; keys 1 hostname ("HALO SERVER" when
//   unnamed), 3 game version, 5 map (the file name of the server's map path), 6 game type (CTF / Slayer / Oddball /
//   King / Race), 7 variant name, 8 players, 10 maximum players (at least 1), 11 mode ("exiting" while closing, else
//   "openplaying"), 12 team play, 13 score limit, 19 password, 0x33 the server flag bit 2, 0x34 the packed custom
//   options, 0x35 the packed game type options, 0x36 bit 7 of 0x006f1cc0; every other key is empty.
// blam-cc: cdecl (a qr2 server key callback)
```

## network_session_host_qr2_team_key.c

```
// network_session_host_qr2_team_key  (not a Ghidra function; no C existed -- the host's qr2 callback, which network_session_host_start passed as NULL)
// address 0x577f40, size 110 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x577f40..0x577fad: the team key callback (key, index, buffer, user data): the
//   game engine's +0xa4 hook answers first; key 0x1c is the team name ("Red" for 0, "Blue" for 1 -- as the binary has
//   it, index 1 is "Blue"); anything else is empty.
// blam-cc: cdecl (a qr2 team key callback)
```

## network_session_host_reject_or_cleanup_client.c

```
// network_session_host_reject_or_cleanup_client  (Ghidra: FUN_00575ff0)
// address 0x575ff0, size 162 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x575ff0..0x576091: the host's CD key check for a joining machine:
//   gcd_authenticate_user(game id 0x0069fdfc, local id, ip, challenge, response,
//   network_session_host_cd_key_callback, 0), then the ban list check on the key hash (gcd_getkeyhash, EDI). Not
//   banned: 1. Banned: reason 6 to the machine with that local id (or NULL), the key is disconnected from gcd (every
//   key for local id -1), 0. (Name kept; it authenticates.)
// blam-cc: EAX -> response, ECX -> challenge, EDX -> ip, ESI -> local_id
```

```
#if 0
Original Ghidra decompilation (0x575ff0):

undefined4 FUN_00575ff0(void)

{
  char cVar1;
  int iVar2;
  int *piVar3;
  int unaff_ESI;

  FUN_0061b110(DAT_0069fdfc);
  FUN_0061aa50(DAT_0069fdfc);
  cVar1 = ban_list_check_and_reject_player();
  if (cVar1 == '\0') {
    return 1;
  }
  iVar2 = 0;
  piVar3 = (int *)(DAT_0071c2d4 + 0x414);
  do {
    if (*piVar3 == unaff_ESI) break;
    iVar2 = iVar2 + 1;
    piVar3 = piVar3 + 0x18;
  } while (iVar2 < 0x10);
  FUN_004e0af0(DAT_0071c2d4);
  if (unaff_ESI != -1) {
    FUN_0061b350(DAT_0069fdfc);
    return 0;
  }
  FUN_0061b3f0(DAT_0069fdfc);
  return 0;
}
#endif
```

## network_session_host_start.c

```
// network_session_host_start  (Ghidra: FUN_00577850)
// address 0x577850, size 159 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x577850..0x5778ee: disposes the old session, opens the channels and starts
//   query/report on the game socket's SOCKET (0x6175f0) with the port, game name and secret key strings, the public
//   flag byte, natneg on, the six host callbacks (the earlier version passed NULL for five of them and the player key
//   callback in the wrong slot) and the argument as user data; registers the natneg callback, and initializes the CD
//   key server with game id 0x319 on the same record. Returns qr2_init_socketA's result.
// blam-cc: cdecl
```

```
#if 0
Original Ghidra decompilation (0x577850):

undefined4 FUN_00577850(undefined4 param_1)

{
  undefined4 uVar1;

  FUN_005778f0();
  network_channels_open();
  uVar1 = FUN_006175f0(DAT_006f14c4);
  uVar1 = FUN_00616340(&DAT_00722a20,uVar1,DAT_007227b8,&DAT_00722798,&DAT_007227a0,DAT_0069fe00,1,
                       &LAB_005779c0,FUN_00577e40,&LAB_00577f40,&LAB_00577fb0,&LAB_005780c0,
                       &LAB_00578100,param_1);
  FUN_00615530(DAT_00722a20,&LAB_00578160);
  DAT_0069fdfc = 0x319;
  FUN_0061b6d0(DAT_00722a20,0x319,DAT_0069fe00);
  return uVar1;
}
#endif
```

## network_session_host_start_info_set.c

```
// network_session_host_start_info_set  (Ghidra: FUN_00576100; named per this rewrite)
// address 0x576100, size 126 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: strings "dedicated", "player_flags", "game_flags", "game_classic"; out/phase4/
// networking_functions.md summary: "Stores host/map/variant name strings and a game-type value
// into globals used to start a network session, and registers well-known Halo script globals
// (dedicated, player_flags, game_flags, game_classic...)."
// register convention: host name in EAX (in_EAX), map name in ESI (unaff_ESI), variant name in
// EDI (unaff_EDI, may be NULL), game type as the recognized stack parameter.
// blam-cc: EAX -> host_name, ESI -> map_name, EDI -> variant_name, stack -> game_type
```

```
#if 0
Original Ghidra decompilation (0x576100):

void FUN_00576100(undefined4 param_1)

{
  char cVar1;
  char *in_EAX;
  int iVar2;
  char *unaff_ESI;
  char *unaff_EDI;

  iVar2 = (int)&DAT_00722798 - (int)in_EAX;
  do {
    cVar1 = *in_EAX;
    in_EAX[iVar2] = cVar1;
    in_EAX = in_EAX + 1;
  } while (cVar1 != '\0');
  iVar2 = (int)&DAT_007227a0 - (int)unaff_ESI;
  do {
    cVar1 = *unaff_ESI;
    unaff_ESI[iVar2] = cVar1;
    unaff_ESI = unaff_ESI + 1;
  } while (cVar1 != '\0');
  if (unaff_EDI != (char *)0x0) {
    iVar2 = (int)&DAT_007227a8 - (int)unaff_EDI;
    do {
      cVar1 = *unaff_EDI;
      unaff_EDI[iVar2] = cVar1;
      unaff_EDI = unaff_EDI + 1;
    } while (cVar1 != '\0');
  }
  DAT_007227b8 = param_1;
  FUN_0061bb40(0x33,"dedicated");
  FUN_0061bb40(0x34,"player_flags");
  FUN_0061bb40(0x35,"game_flags");
  FUN_0061bb40(0x36,"game_classic");
  return;
}
#endif
```

## network_session_host_update.c

```
// network_session_host_update  (Ghidra: FUN_00577940; named per this rewrite)
// address 0x577940, size 120 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary: "Periodic per-frame update for the
// network channel/session object: flushes it on timeout or close request, then pumps it."
// register convention: no register-passed arguments.
// UNSURE: FUN_00449210 (a tick/timer read), qr2_send_statechanged and qr2_think (foreign,
// GameSpy-shaped, argument lists elided).
```

```
#if 0
Original Ghidra decompilation (0x577940):

void FUN_00577940(void)

{
  int iVar1;

  if (DAT_00722a20 != 0) {
    if ((DAT_00722a18 != 0) &&
       ((iVar1 = FUN_00449210(), DAT_00722a18 == 2 || (999 < (uint)(iVar1 - DAT_00722a24))))) {
      DAT_00722a1c = DAT_00722a18 == 2;
      FUN_00616c00(DAT_00722a20);
      DAT_00722a18 = 0;
      DAT_00722a24 = iVar1;
    }
    FUN_00616cb0(DAT_00722a20);
    DAT_00722a1c = 0;
  }
  return;
}
#endif
```
