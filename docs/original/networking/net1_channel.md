# Original notes: networking `net1_channel`

Author notes and decompile blocks moved verbatim from the C files converted into the net1_channel sources.

## network_buffer_pair_pool_clear.c

```
// network_buffer_pair_pool_clear  (Ghidra: FUN_004e3ed0; renamed -- see evidence)
// address 0x4e3ed0, size 89 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md ("Frees every entry of a small array of paired
// GlobalAlloc allocations and resets its bookkeeping globals, most likely tearing down a cached
// list of dynamically-allocated network buffers"). No caller was found in this batch or
// elsewhere in the codebase (out/functions.json lists one caller, outside this batch's evidence
// window), so the pool's producer/consumer is not identified.
// register convention: __cdecl, no arguments.
// UNSURE: this function's exact relationship to the growable_array-shaped ban_list bookkeeping
// (0x006b859c region, see network_banlist_save.c) is not established; the three globals here
// (0x006b85a8/ac/b0) are a separate three-field record (an element-size-like sentinel, a count,
// and a data pointer to size*8 bytes) that happens to sit right after ban_list's own fields in
// memory, but nothing in this batch proves they are related.
```

```
#if 0
Original Ghidra decompilation (0x4e3ed0), from tools/pack.py 0x4e3ed0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004e3ed0(void)

{
  HGLOBAL pvVar1;
  int iVar2;

  iVar2 = 0;
  if (0 < DAT_006b85ac) {
    do {
      pvVar1 = DAT_006b85b0;
      GlobalFree(*(HGLOBAL *)((int)DAT_006b85b0 + iVar2 * 8));
      GlobalFree(*(HGLOBAL *)((int)pvVar1 + iVar2 * 8 + 4));
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_006b85ac);
  }
  _DAT_006b85a8 = 0xffffffff;
  DAT_006b85ac = 0xffffffff;
  if (DAT_006b85b0 != (HGLOBAL)0x0) {
    GlobalFree(DAT_006b85b0);
    DAT_006b85b0 = (HGLOBAL)0x0;
  }
  return;
}
#endif
```

## network_channel_attempt_connect.c

```
// network_channel_attempt_connect  (Ghidra: FUN_00441f60, still unnamed -> renamed)
// address 0x441f60, size 214 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("attempts to (re)establish a network
// channel connection, flagging a disconnect/error state and notifying elsewhere if the attempt
// fails"); the queue fields written (+0x05 unknown_05, +0x0e last_error) match
// network_receive_queue; the address fields read (+0x00 ipv4, +0x12 port) match
// s_network_address; 0x0071c2de matches types/networking.h's documented host-handoff request
// flag.
// register convention: s_network_address * in EAX (in_EAX), network_receive_queue * in ESI
// (unaff_ESI) -- neither is this function's own declared parameter, both are threaded through
// from callers exactly like network_channel_get_remote_address.c and
// network_channel_receive_callback.c in this same file group; unused_param_1 and
// use_query_socket are genuine stack parameters (confirmed by the only two call sites, both in
// out/phase2/networking/01.md, passing literal (0x96640, 1)).
// UNSURE: unused_param_1 (0x96640 at both call sites) is never read anywhere in this function's
// body; kept as an unused parameter exactly as decompiled rather than dropped.
// FIXED in the review pass, all three from the disassembly at 0x441ff5..0x442035:
//   - the function returns in AX, not EAX (mov ax,bx / mov ax,di with edi = 0xfffffff0), so
//     the failure result is the int16_t -16, not the int32_t 0xfff0 an earlier draft returned.
//     types/networking.h now carries it as k_network_error_connect_failed.
//   - 0x00718fa4 is accessed with WORD PTR (cmp .,0xffff / mov .,0x7), so it is an int16_t.
//   - gt2SetConnectionData takes two arguments here as well: 0x441fe6 is
//     mov eax,[esi]; push esi; push eax, i.e. (queue->socket, queue), matching
//     network_listen_accept_pending_connection.c's call site.
```

```
#if 0
Original Ghidra decompilation (0x441f60):

undefined4 FUN_00441f60(undefined4 param_1,char param_2)

{
  undefined4 *in_EAX;
  undefined4 uVar1;
  int iVar2;
  undefined4 *unaff_ESI;
  undefined1 local_18 [24];

  uVar1 = FUN_00614890(*in_EAX);
  FUN_006148b0(uVar1,*(undefined2 *)((int)in_EAX + 0x12),local_18);
  network_channels_open();
  iVar2 = DAT_006f14c8;
  if (param_2 == '\0') {
    iVar2 = DAT_006f14c4;
  }
  if (iVar2 != 0) {
    iVar2 = FUN_006145a0(iVar2);
    if (iVar2 == 0) {
      *(undefined1 *)((int)unaff_ESI + 5) = 0;
      FUN_00614830(*unaff_ESI);
      *(undefined2 *)((int)unaff_ESI + 0xe) = 0;
      return 0;
    }
  }
  *(undefined1 *)((int)unaff_ESI + 5) = 1;
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 7;
  }
  DAT_0071c2de = 1;
  chat_close();
  *(undefined2 *)((int)unaff_ESI + 0xe) = 0xfff0;
  return 0xfff0;
}
#endif
```

## network_channel_connected_callback.c

```
// network_channel_connected_callback  (no prior name; the GT2 "connected" callback)
// address 0x441e00, size 201 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 (retail-independence loop) from the disassembly 0x441e00..0x441ec9. It had no C because it is
// reached only as an immediate: network_channel_attempt_connect (0x441f60) stores it as the `connected` member of
// the GT2ConnectionCallbacks it hands gt2Connect (with network_channel_receive_callback 0x441ed0,
// network_channel_gap_441f30 and function_do_nothing 0x44ad80). gt2 calls it with (connection, result, message,
// length) once the connect attempt resolves: the connection's data is the channel's receive queue
// (gt2GetConnectionData); the remote address is resolved and formatted (for the log); then
//   result 0 (connected): no error, flags |= 0x31 (connection oriented, readable, ...)
//   result 2 (rejected): +0x18 = the reject reason from a 4-byte message when it is 1..8 (else 1), +0x05 = 1, error
//                        -0x18, socket closed, flags |= 0x40
//   result 6:            error -0x10, socket closed, flags |= 0x80
//   anything else:       error -0x10, socket closed.
// blam-cc: cdecl (a GT2 callback)
```

## network_channel_delete.c

```
// network_channel_delete  (Ghidra: network_channel_delete, already named)
// address 0x4dcae0, size 329 bytes
// name confidence: 0.55   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Tears down and frees a network channel object,
// recursively closing/freeing any child channels, its buffers, and associated OS handles before
// freeing the object itself." Every dword-indexed offset (channel[3]=0x00c, channel+0x2a3(byte)=
// 0xa8c, channel+0x2a8=0xaa0, channel[0x2a7]=0xa9c, channel+0xb(byte)=0x02c, channel[4..10]=
// 0x010..0x028, channel+0x158(byte)=0x560, channel[0x151..0x157]=0x544..0x55c,
// channel[0x29e]/[0x29f]/[0x2a0]=0xa78/0xa7c/0xa80) matches types/networking.h's network_channel
// field offsets exactly (incoming, flags, children, listen_list, in.empty, in.stream.*,
// out.empty, out.stream.*, reliable_count, reliable, send_budget), for both the listening and
// plain variants.

// VERIFIED against disassembly 0x4dcae0..0x4dcc29 (2026-09-30): endpoint freed via EAX only; listen list, both streams, reliable slots and send_budget compared field by field
```

```
#if 0
Original Ghidra decompilation (0x4dcae0):

void __cdecl network_channel_delete(int *channel)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  code *pcVar4;

  if (channel != (int *)0x0) {
    if (*channel != 0) {
      network_receive_queue_free();
    }
    pcVar4 = GlobalFree_exref;
    if ((HGLOBAL)channel[3] != (HGLOBAL)0x0) {
      GlobalFree((HGLOBAL)channel[3]);
    }
    if ((*(byte *)(channel + 0x2a3) & 1) != 0) {
      piVar1 = channel + 0x2a8;
      if (piVar1 != (int *)0x0) {
        iVar2 = 0x10;
        do {
          if ((undefined4 *)*piVar1 != (undefined4 *)0x0) {
            if (channel[0x2a7] != 0) {
              network_channel_list_remove(*(undefined4 *)*piVar1);
            }
            network_channel_delete((int *)*piVar1);
          }
          piVar1 = piVar1 + 1;
          iVar2 = iVar2 + -1;
          pcVar4 = GlobalFree_exref;
        } while (iVar2 != 0);
      }
      iVar2 = channel[0x2a7];
      if (iVar2 != 0) {
        (*pcVar4)(*(undefined4 *)(iVar2 + 0x104));
        (*pcVar4)(iVar2);
      }
    }
    *(undefined1 *)(channel + 0xb) = 1;
    channel[5] = 0;
    channel[6] = 0;
    channel[7] = 0;
    channel[8] = 0;
    channel[9] = 0;
    channel[10] = 0;
    channel[4] = -1;
    *(undefined1 *)(channel + 0x158) = 1;
    channel[0x151] = -1;
    channel[0x152] = 0;
    channel[0x153] = 0;
    channel[0x154] = 0;
    channel[0x155] = 0;
    channel[0x156] = 0;
    channel[0x157] = 0;
    if (0 < channel[0x29e]) {
      iVar2 = 0;
      if (0 < channel[0x29e]) {
        iVar3 = 0;
        do {
          GlobalFree(*(HGLOBAL *)(channel[0x29f] + 0x18 + iVar3));
          GlobalFree(*(HGLOBAL *)(channel[0x29f] + 0x1c + iVar3));
          iVar2 = iVar2 + 1;
          iVar3 = iVar3 + 0x20;
          pcVar4 = GlobalFree_exref;
        } while (iVar2 < channel[0x29e]);
      }
      (*pcVar4)(channel[0x29f]);
      channel[0x29f] = 0;
      channel[0x29e] = 0;
    }
    channel[0x2a0] = 0;
    (*pcVar4)(channel);
  }
  return;
}
#endif
```

## network_channel_gap_441020.c

```
// network_channel_gap_441020  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x441020, size 29 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x441020..0x44103c: the GT2 receive dump callback set by network_channels_open
//   (socket, connection, ip, port, reset, message, length, reliable, resend): records the packet with is_sent 1 and
//   the last two arguments as reliable / resend.
// blam-cc: cdecl (a GT2 dump callback)
```

## network_channel_gap_441040.c

```
// network_channel_gap_441040  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x441040, size 23 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x441040..0x441056: the GT2 send dump callback set by network_channels_open:
//   records the packet with is_sent, reliable and resend all 0.
// blam-cc: cdecl (a GT2 dump callback)
```

## network_channel_gap_441060.c

```
// network_channel_gap_441060  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x441060, size 77 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x441060..0x4410ac: the GT2 socket-error callback network_channels_open gives
//   both sockets: an unset join error (-1) becomes 6, a host handoff is requested, chat closes, every connection on
//   the socket is closed (gt2CloseAllConnections 0x614740, soft) and the socket global that held it -- the game
//   socket when it is that one, otherwise the query socket -- is cleared (GT2 frees the socket after this returns).
// blam-cc: cdecl (a GT2 socket error callback)
```

## network_channel_gap_4410b0.c

```
// network_channel_gap_4410b0  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x4410b0, size 331 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4410b0..0x4411fa: the game socket's GT2 unrecognized-message callback (socket,
//   ip, port, message, length): like 0x441200 (copy of at most 0x1fff bytes to 0x006a4140; natneg packets to
//   NNProcessData and handled) but a query ("\\" or ";" first, or 0xfe 0xfd) also goes to qr2_parse_queryA for the
//   host record when there is one; queries count as handled, anything else 0.
// blam-cc: cdecl (a GT2 unrecognized message callback)
```

## network_channel_gap_441200.c

```
// network_channel_gap_441200  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x441200, size 243 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x441200..0x4412f2: the query socket's GT2 unrecognized-message callback (socket,
//   ip, port, message, length): the message (at most 0x1fff bytes) is copied to 0x006a6148 and terminated; a natneg
//   packet (the 6 byte magic at 0x00657208) goes to NNProcessData with the sender as a sockaddr_in and is handled
//   (1); a query ("\\" or ";" first, or 0xfe 0xfd) counts as handled (1); anything else 0.
// blam-cc: cdecl (a GT2 unrecognized message callback)
```

## network_channel_gap_441f30.c

```
// network_channel_gap_441f30  (not a Ghidra function; no C existed)
// address 0x441f30, size 35 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x441f30..0x441f52: the GT2 connection callback set by
//   network_listen_accept_pending_connection (config.error_callback): the connection's receive queue
//   (gt2GetConnectionData) gets byte +5 = 1, network_receive_queue_close_socket (ESI queue) and flag 0x40 in +0x0c.
//   (The name keeps the one its registrant uses.)
// blam-cc: cdecl (a GT2 connection callback)
```

## network_channel_gap_4ba660.c

```
// network_channel_gap_4ba660  (not a Ghidra function; no C existed)
// address 0x4ba660, size 225 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ba660..0x4ba740: the ServerBrowser list callback (sb, reason, server,
//   instance) registered by server_browser_open, active once the browser is initialized: server added (0) with basic
//   or full keys, and server updated (1), add the server to the locked list (0x4ba760 / 0x4ba8a0); deleted (2, 3)
//   removes it (0x4ba870 / 0x4ba940), dropping the selection and refreshing the UI when it was selected; query
//   complete (4) resets the elapsed time to 9999 when a query was pending (0x00719488). (Named as its registrant
//   names it.)
// blam-cc: cdecl (a serverbrowsing callback)

// FIXED 2026-09-28: DAT_00719488 here is the global at its address comment, server_browser_query_pending (the name belonged to another
// global at a different address, so the link bound it there).
```

## network_channel_get_remote_address.c

```
// network_channel_get_remote_address  (Ghidra: network_channel_get_remote_address, already named)
// address 0x441ce0, size 276 bytes
// name confidence: 0.6   rewrite confidence: 0.5
// evidence: out/phase4/networking_types_notes.md's s_network_address section and
// network_error_code enum: the out-parameter's fields (dword at +0x00, word at +0x12, word at
// +0x10 set to 4, i.e. k_network_address_size_ipv4) and the -15/0xfff1 error code
// (k_network_error_no_address) line up exactly; the queue pointer's +0x00 (socket) and +0x0e
// (last_error) fields match network_receive_queue.
// FIXED in the review pass: Ghidra declared this void, but it returns its error code in EAX --
// 0 on both success paths (0x441d4f / 0x441dc9 reload the zero from [esp+0x8]) and 0xfffffff1
// (k_network_error_no_address) at 0x441de1 -- and 0x4dd390 tests exactly that (`test ax,ax`).
// The same value is also written to queue->last_error, which is why the void model still
// behaved correctly at the one call site that checked it.
// register convention: out s_network_address * in ESI (unaff_ESI), network_receive_queue * in
// EDI (unaff_EDI).
// UNSURE: gamespy_array_length/gt2GetLocalIP are each called four times with the identical argument and
// their four results are combined with a mask/shift expression that is algebraically a full
// 32-bit byte-swap of a single such call's result (consistent with converting a network-order
// GameSpy address into this struct's host-order-high-byte-first layout) -- kept as four
// separate calls, exactly as decompiled, rather than collapsed into one, since these are
// genuine calls whose independence was not confirmed.
```

```
#if 0
Original Ghidra decompilation (0x441ce0):

void network_channel_get_remote_address(void)

{
  undefined2 uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint *unaff_ESI;
  int *unaff_EDI;

  if (*unaff_EDI != 0) {
    iVar2 = FUN_006147a0(*unaff_EDI);
    if (iVar2 == 1) {
      uVar3 = FUN_006175f0(*unaff_EDI);
      uVar4 = FUN_006175f0(*unaff_EDI);
      iVar2 = FUN_006175f0(*unaff_EDI);
      uVar5 = FUN_006175f0(*unaff_EDI);
      *unaff_ESI = (uVar3 & 0xff0000 | uVar4 >> 0x10) >> 8 | (iVar2 << 0x10 | uVar5 & 0xff00) << 8;
      uVar1 = FUN_006147d0(*unaff_EDI);
      *(undefined2 *)((int)unaff_ESI + 0x12) = uVar1;
      *(undefined2 *)(unaff_ESI + 4) = 4;
      *(undefined2 *)((int)unaff_EDI + 0xe) = 0;
      return;
    }
  }
  if (DAT_006f14c4 != 0) {
    uVar3 = FUN_006147e0(DAT_006f14c4);
    uVar4 = FUN_006147e0(DAT_006f14c4);
    iVar2 = FUN_006147e0(DAT_006f14c4);
    uVar5 = FUN_006147e0(DAT_006f14c4);
    *unaff_ESI = (uVar3 & 0xff0000 | uVar4 >> 0x10) >> 8 | (iVar2 << 0x10 | uVar5 & 0xff00) << 8;
    uVar1 = FUN_006147f0(DAT_006f14c4);
    *(undefined2 *)((int)unaff_ESI + 0x12) = uVar1;
    *(undefined2 *)(unaff_ESI + 4) = 4;
    *(undefined2 *)((int)unaff_EDI + 0xe) = 0;
    return;
  }
  *unaff_ESI = 0;
  *(undefined2 *)((int)unaff_ESI + 0x12) = 0;
  *(undefined2 *)(unaff_ESI + 4) = 4;
  *(undefined2 *)((int)unaff_EDI + 0xe) = 0xfff1;
  return;
}
#endif
```

## network_channel_incoming_read_item.c

```
// network_channel_incoming_read_item  (Ghidra: FUN_004dcf10; named per this rewrite)
// address 0x4dcf10, size 372 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Reads one length-prefixed item out of the
// channel's incoming ring buffer, used by the incoming-message processing loop before each item
// is dispatched." Peeks 2 bytes from channel->incoming, decodes a bit-chunked length prefix from
// them, validates the length against both a hidden byte-count bound and the buffer's actual
// available bytes, then (on success) consumes the item into `destination` and reports the
// decoded length via out_bit_offset/out_remaining_bits, resetting the buffer's cursors to 0 on
// any failure path.
// max_item_bits arrives in EAX (mov esi,eax at entry) and is the bound the decoded byte length is
// checked against. The local bit_stream is 6 dwords at [esp+0x10..0x27] (unknown_00 = 1,
// data = the 2 peeked bytes, last_bit = 0xf); item_length sits right before it and is the buffer
// the 16-bit chunk is read into.
// register/parameter convention: max_item_bits in EAX (elided). blam-cc: EAX -> max_item_bits,
// stack -> channel, destination, out_bit_offset, out_remaining_length_bits, out_address

// VERIFIED against disassembly 0x4dcf10..0x4dd084 (2026-09-30): FIXED: bit count passed to the chunked read is network_bit_chunk_size (EAX=edi), ceil(bits/8) uses signed / and %, address is cleared when network_channel_get_remote_address returns nonzero (test ax,ax), 6th dword at +0x14 also zeroed
```

```
#if 0
Original Ghidra decompilation (0x4dcf10):

undefined4
FUN_004dcf10(int param_1,undefined4 param_2,int *param_3,int *param_4,undefined4 *param_5)

{
  char cVar1;
  short sVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  undefined1 local_24 [4];
  int local_20;
  undefined4 local_1c;
  undefined1 *local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar7 = *(int *)(param_1 + 0xc);
  if (iVar7 != 0) {
    iVar3 = *(int *)(iVar7 + 0xc) - *(int *)(iVar7 + 8);
    if (iVar3 < 0) {
      iVar3 = iVar3 + *(int *)(iVar7 + 0x10);
    }
    if (0 < iVar3) {
      local_20 = 0;
      cVar1 = circular_buffer_read(local_24,2,0);
      iVar7 = DAT_0071c2cc;
      if (cVar1 != '\0') {
        local_18 = local_24;
        local_c = 0;
        local_10 = 0;
        local_1c = 1;
        local_14 = 0;
        local_8 = 0xf;
        local_4 = 0x10;
        iVar4 = bit_stream_read_bits_chunked(&local_1c);
        iVar3 = local_20;
        if (iVar4 != iVar7) {
          return 0;
        }
        if (1 < local_20) {
          uVar6 = in_EAX & 0x80000007;
          if ((int)uVar6 < 0) {
            uVar6 = (uVar6 - 1 | 0xfffffff8) + 1;
          }
          if (local_20 <=
              (int)((uint)(uVar6 != 0) + ((int)(in_EAX + ((int)in_EAX >> 0x1f & 7U)) >> 3))) {
            iVar4 = *(int *)(param_1 + 0xc);
            iVar5 = *(int *)(iVar4 + 0xc) - *(int *)(iVar4 + 8);
            if (iVar5 < 0) {
              iVar5 = iVar5 + *(int *)(iVar4 + 0x10);
            }
            if (local_20 <= iVar5) {
              circular_buffer_read(param_2,local_20,1);
              if (param_5 != (undefined4 *)0x0) {
                sVar2 = network_channel_get_remote_address();
                iVar7 = DAT_0071c2cc;
                if (sVar2 != 0) {
                  *param_5 = 0;
                  param_5[1] = 0;
                  param_5[2] = 0;
                  param_5[3] = 0;
                  param_5[4] = 0;
                  param_5[5] = 0;
                  *(undefined2 *)(param_5 + 4) = 4;
                }
              }
              *param_3 = iVar7;
              *param_4 = iVar3 * 8 - iVar7;
              return 1;
            }
          }
        }
        iVar7 = *(int *)(param_1 + 0xc);
        *(undefined4 *)(iVar7 + 0xc) = 0;
        *(undefined4 *)(iVar7 + 8) = 0;
        return 0;
      }
    }
  }
  return 0;
}
#endif
```

## network_channel_key_close.c

```
// network_channel_key_close  (Ghidra: FUN_004de8c0; named per this rewrite)
// address 0x4de8c0, size 55 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Releases the channel previously obtained for
// the given (player,machine) key via player_new_local, recording the returned index at in_EAX+0x1f
// if valid." Mirrors network_channel_key_open.c.
// register convention: EAX -> entry, EBX -> requested_handle.
// blam-cc: EAX -> entry, EBX -> requested_handle
// FIXED (register inputs, objdump): EBX (read at 0x4de8dd, "mov eax,ebx" right before the call)
// is not an implicit result of player_new_local -- it is player_new_local's own EAX/
// requested_handle register argument (confirmed against src/game/player_new_local.c's recovered
// signature: EAX -> requested_handle, stack -> machine_index, local_player_index,
// identifier_record). EBX is callee-saved in cdecl, so it is unchanged by the call, and the
// `cmp ebx,0xffffffff` afterward re-examines this function's own EBX input, not player_new_local's
// (unused) EAX return value. The previous header's guess that EBX was written by the call was
// wrong. Also corrected while fixing this: `esi` (entry) is pushed directly as
// player_new_local's identifier_record argument (0x4de8d6, `push esi`), not a local &index
// out-param -- the previous rewrite invented a 3-argument player_new_local call with an
// out-parameter that objdump does not support.
```

```
#if 0
Original Ghidra decompilation (0x4de8c0):

undefined4 FUN_004de8c0(void)

{
  char cVar1;
  short sVar2;
  int in_EAX;
  int unaff_EBX;

  cVar1 = FUN_004ddcc0();
  if (cVar1 == '\0') {
    sVar2 = -1;
  }
  else {
    sVar2 = (short)*(char *)(in_EAX + 0x1d);
  }
  FUN_00473940((int)*(char *)(in_EAX + 0x1c),sVar2);
  if (unaff_EBX != -1) {
    *(char *)(in_EAX + 0x1f) = (char)unaff_EBX;
    return 1;
  }
  return 0;
}
#endif
```

## network_channel_key_open.c

```
// network_channel_key_open  (Ghidra: FUN_004de870; named per this rewrite)
// address 0x4de870, size 75 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Opens/obtains a network channel for the given
// (player,machine) key via player_new_network, records the resulting index, and notifies network_index_cache_find_or_allocate_slot
// of the new channel." entry->machine_index/machine_player_index/slot_index match
// types/networking.h's network_player_entry.
// register convention: entry in EAX (in_EAX). blam-cc: EAX -> entry
```

```
#if 0
Original Ghidra decompilation (0x4de870):

undefined4 FUN_004de870(void)

{
  char cVar1;
  short sVar2;
  int in_EAX;
  int iVar3;

  cVar1 = FUN_004ddcc0();
  if (cVar1 == '\0') {
    sVar2 = -1;
  }
  else {
    sVar2 = (short)*(char *)(in_EAX + 0x1d);
  }
  iVar3 = FUN_00473780((int)*(char *)(in_EAX + 0x1c),sVar2);
  if (iVar3 != -1) {
    *(char *)(in_EAX + 0x1f) = (char)iVar3;
    FUN_004e9c20(iVar3);
    return 1;
  }
  return 0;
}
#endif
```

## network_channel_key_resolve_target.c

```
// network_channel_key_resolve_target  (Ghidra: FUN_004ddcc0; named per this rewrite)
// address 0x4ddcc0, size 81 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Decides whether the current channel-key entry
// (ESI) should resolve to a specific machine slot or a wildcard/broadcast target, based on the
// network-game state and DAT_0071c2d4/DAT_0071c2d8 globals." +0x1c on the ESI object matches
// network_player_entry.machine_index (see network_game_session_reset.c and
// network_player_entry_add.c for the evidence this whole 0x4de390..0x4de950 cluster shares).
// UNSURE (significant): `*DAT_0071c2d8` dereferences network_client as if it were an `int *`
// (its own first field, unknown_000, is only a uint16_t per types/networking.h); this rewrite
// preserves that exact dereference via a raw cast rather than asserting a specific meaning.
// register convention: entry in ESI (unaff_ESI). blam-cc: ESI -> entry
```

```
#if 0
Original Ghidra decompilation (0x4ddcc0):

bool FUN_004ddcc0(void)

{
  char cVar1;
  int unaff_ESI;

  if ((unaff_ESI == 0) || (cVar1 = network_player_entry_validate(), cVar1 == '\0')) {
    if (DAT_00719720 != 3) {
      return true;
    }
    return *(char *)(unaff_ESI + 0x1c) == '\0';
  }
  if (((DAT_0071c2d4 == 0) || ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) == 0)) &&
     ((*DAT_0071c2d8 != -1 && ((int)*DAT_0071c2d8 == (int)*(char *)(unaff_ESI + 0x1c))))) {
    return true;
  }
  return false;
}
#endif
```

## network_channel_key_send_state.c

```
// network_channel_key_send_state  (Ghidra: FUN_004de950; named per this rewrite)
// address 0x4de950, size 148 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md: "Builds and sends a large state packet (via
// FUN_004ec590/network_game_settings_packet_receive) when the connection is mid-game as host or client, otherwise
// delegates to FUN_004ec670." client->state (state 2 or 3) matches this cluster's established
// field.
// UNSURE: `entry` (in_EDX, a pointer-to-pointer) and the ~940-byte zeroed scratch buffer passed
// to FUN_004ec590/network_game_settings_packet_receive are not independently identified; declared generically.
// register convention: client in ESI (unaff_ESI), entry in EDX (in_EDX). blam-cc: EDX -> entry,
// ESI -> client
```

```
#if 0
Original Ghidra decompilation (0x4de950):

undefined4 FUN_004de950(void)

{
  char cVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 *in_EDX;
  int unaff_ESI;
  undefined4 *puVar4;
  undefined2 local_3b8;
  undefined4 local_3b6 [236];

  if ((DAT_00719720 == 2) ||
     ((*(short *)(unaff_ESI + 0xeda) != 2 && (*(short *)(unaff_ESI + 0xeda) != 3)))) {
    uVar2 = FUN_004ec670();
    return uVar2;
  }
  if (*(int *)*in_EDX == 0) {
    local_3b8 = 0;
    puVar4 = local_3b6;
    for (iVar3 = 0xeb; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    *(undefined2 *)puVar4 = 0;
    cVar1 = FUN_004ec590();
    if (cVar1 != '\0') {
      cVar1 = FUN_004d9800(&local_3b8);
      if (cVar1 != '\0') {
        return 1;
      }
      return 0;
    }
  }
  return 0;
}
#endif
```

## network_channel_list_add.c

```
// network_channel_list_add  (Ghidra: network_channel_list_add, already named)
// address 0x441a40, size 184 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md "network_channel_list (0x114)" section:
// "network_channel_list_add uses +0x00 as a count, +0x04.. as a 64-entry dedup array and
// +0x10c as the write cursor."
// register convention: entry pointer in EAX (in_EAX), list pointer in ECX (in_ECX).
// UNSURE: the decompiled `if ((entry->flags & 2) == 0) { X } else { X }` has byte-for-byte
// identical bodies in both branches; reproduced literally (as two identical blocks) rather
// than collapsed, since this rewrite does not assume that duplication is meaningless dead
// code versus a compiled remnant of genuinely different source.
```

```
#if 0
Original Ghidra decompilation (0x441a40):

undefined4 network_channel_list_add(void)

{
  int iVar1;
  uint uVar2;
  int in_EAX;
  uint *in_ECX;
  uint uVar3;
  uint *puVar4;

  if (((int)(in_ECX[0x42] - 1) < (int)in_ECX[0x43]) || (iVar1 = in_ECX[0x43] + 1, iVar1 < 0)) {
    return 0xffffffec;
  }
  *(int *)(in_ECX[0x41] + iVar1 * 4) = in_EAX;
  uVar2 = *in_ECX;
  if ((*(byte *)(in_EAX + 0xc) & 2) == 0) {
    uVar3 = 0;
    if (uVar2 != 0) {
      puVar4 = in_ECX;
      do {
        puVar4 = puVar4 + 1;
        if (*puVar4 == *(uint *)(*(int *)(in_ECX[0x41] + iVar1 * 4) + 8)) break;
        uVar3 = uVar3 + 1;
      } while (uVar3 < *in_ECX);
    }
  }
  else {
    uVar3 = 0;
    if (uVar2 != 0) {
      puVar4 = in_ECX;
      do {
        puVar4 = puVar4 + 1;
        if (*puVar4 == *(uint *)(*(int *)(in_ECX[0x41] + iVar1 * 4) + 8)) break;
        uVar3 = uVar3 + 1;
      } while (uVar3 < *in_ECX);
    }
  }
  if ((uVar3 == uVar2) && (uVar2 < 0x40)) {
    in_ECX[uVar3 + 1] = *(uint *)(*(int *)(in_ECX[0x41] + iVar1 * 4) + 8);
    *in_ECX = *in_ECX + 1;
  }
  in_ECX[0x43] = in_ECX[0x43] + 1;
  *(byte *)(in_EAX + 0xc) = *(byte *)(in_EAX + 0xc) | 8;
  return 0;
}
#endif
```

## network_channel_list_mark_readable.c

```
// network_channel_list_mark_readable  (Ghidra: FUN_004419d0, still unnamed -> renamed)
// address 0x4419d0, size 102 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("iterates a channel list, marking
// each channel that has data ready to read (either explicitly flagged or via a nonzero
// circular-buffer fill level)"); out/phase4/networking_types_notes.md pins
// network_pending_connection_count (0x006f16d0) and network_receive_queue's flags/incoming
// fields.
// register convention: list pointer in EDI (unaff_EDI).
// UNSURE: exactly why a nonzero network_pending_connection_count alone (regardless of actual
// buffered bytes) is enough to mark a data_ready entry readable is not explained here; kept
// exactly as decompiled.
```

```
#if 0
Original Ghidra decompilation (0x4419d0):

undefined4 FUN_004419d0(void)

{
  byte *pbVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  int unaff_EDI;

  iVar3 = DAT_006f16d0;
  iVar6 = 0;
  uVar4 = 0xfffffff3;
  if (-1 < *(int *)(unaff_EDI + 0x10c)) {
    do {
      iVar2 = *(int *)(*(int *)(unaff_EDI + 0x104) + iVar6 * 4);
      if ((*(char *)(iVar2 + 4) == '\x01') && (0 < iVar3)) {
LAB_00441a19:
        pbVar1 = (byte *)(*(int *)(*(int *)(unaff_EDI + 0x104) + iVar6 * 4) + 0xc);
        *pbVar1 = *pbVar1 | 4;
        uVar4 = 0;
      }
      else {
        iVar2 = *(int *)(iVar2 + 0x10);
        if (iVar2 != 0) {
          iVar5 = *(int *)(iVar2 + 0xc) - *(int *)(iVar2 + 8);
          if (iVar5 < 0) {
            iVar5 = iVar5 + *(int *)(iVar2 + 0x10);
          }
          if (0 < iVar5) goto LAB_00441a19;
        }
      }
      iVar6 = iVar6 + 1;
    } while (iVar6 <= *(int *)(unaff_EDI + 0x10c));
  }
  return uVar4;
}
#endif
```

## network_channel_list_new.c

```
// network_channel_list_new  (Ghidra: FUN_00441960, still unnamed -> renamed)
// address 0x441960, size 109 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: out/phase4/networking_types_notes.md "network_channel_list (0x441960 allocate,
// 0x441a40 add, 0x441b00 remove, 0x4419d0 the mark-readable pass)": every field this function
// writes (fd_count, entries, capacity, last_index, unknown_110) matches the declared struct,
// and the "constructor refuses more than 0x40" note is this function's `in_AX < 0x41` check.
// register convention: requested capacity in AX, the low 16 bits of EAX (in_AX).
```

```
#if 0
Original Ghidra decompilation (0x441960):

undefined4 * FUN_00441960(void)

{
  short in_AX;
  undefined4 *hMem;
  HGLOBAL pvVar1;

  hMem = GlobalAlloc(0,0x114);
  if (hMem == (undefined4 *)0x0) {
    return (undefined4 *)0x0;
  }
  if (in_AX < 0x41) {
    *hMem = 0;
    pvVar1 = GlobalAlloc(0x40,in_AX * 4);
    hMem[0x41] = pvVar1;
    if (pvVar1 != (HGLOBAL)0x0) {
      hMem[0x42] = (int)in_AX;
      hMem[0x43] = 0xffffffff;
      hMem[0x44] = 0;
      return hMem;
    }
  }
  GlobalFree(hMem);
  return (undefined4 *)0x0;
}
#endif
```

## network_channel_list_remove.c

```
// network_channel_list_remove  (Ghidra: network_channel_list_remove, already named)
// address 0x441b00, size 175 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md "network_channel_list (0x114)" section:
// "network_channel_list_add uses +0x00 as a count, +0x04.. as a 64-entry dedup array and
// +0x10c as the write cursor." This is add's mirror image: it finds `entry` in `list->entries`
// by pointer identity, removes its socket_key from the dedup array (shifting later entries
// down), then swap-removes the entries slot by moving the last live entry into the freed one.
// in ECX (in_ECX), matching network_channel_list_add.c's convention for the same struct.
// UNSURE: none beyond the shared network_channel_list layout notes above.
```

```
#if 0
Original Ghidra decompilation (0x441b00):

undefined4 network_channel_list_remove(int param_1)

{
  undefined4 uVar1;
  uint uVar2;
  uint *in_ECX;
  int *piVar3;
  uint *puVar4;
  int iVar5;

  iVar5 = 0;
  uVar1 = 0xffffffed;
  if (-1 < (int)in_ECX[0x43]) {
    piVar3 = (int *)in_ECX[0x41];
    while (*piVar3 != param_1) {
      iVar5 = iVar5 + 1;
      piVar3 = piVar3 + 1;
      if ((int)in_ECX[0x43] < iVar5) {
        return uVar1;
      }
    }
    uVar2 = 0;
    if (*in_ECX != 0) {
      puVar4 = in_ECX;
LAB_00441b47:
      puVar4 = puVar4 + 1;
      if (*puVar4 != *(uint *)(param_1 + 8)) goto code_r0x00441b4b;
      if (uVar2 < *in_ECX - 1) {
        puVar4 = in_ECX + uVar2 + 1;
        do {
          *puVar4 = puVar4[1];
          uVar2 = uVar2 + 1;
          puVar4 = puVar4 + 1;
        } while (uVar2 < *in_ECX - 1);
      }
      *in_ECX = *in_ECX - 1;
    }
LAB_00441b72:
    *(byte *)(param_1 + 0xc) = *(byte *)(param_1 + 0xc) & 0xf7;
    *(undefined4 *)(in_ECX[0x41] + iVar5 * 4) = *(undefined4 *)(in_ECX[0x41] + in_ECX[0x43] * 4);
    *(undefined4 *)(in_ECX[0x41] + in_ECX[0x43] * 4) = 0;
    in_ECX[0x43] = in_ECX[0x43] - 1;
    uVar1 = 0;
  }
  return uVar1;
code_r0x00441b4b:
  uVar2 = uVar2 + 1;
  if (*in_ECX <= uVar2) goto LAB_00441b72;
  goto LAB_00441b47;
}
#endif
```

## network_channel_listen_service.c

```
// network_channel_listen_service  (Ghidra: FUN_004dd4e0; named per this rewrite)
// address 0x4dd4e0, size 591 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md: "Processes pending incoming-connection and data
// events on a channel, accepting new child connections and forwarding received data to the
// matching existing child channel." listen_list (+0xa9c), its entries/capacity/last_index/
// unknown_110 scan cursor (+0x104/+0x108/+0x10c/+0x110), children[16] (+0xaa0), connected
// (+0xa98), parent (+0xa94) and child_busy (+0xae1) all match types/networking.h exactly.
// UNSURE (significant): `piVar2[2]` (channel->unknown_008, offset +0x008) is called through as a
// function pointer when non-NULL ("accept override callback"), otherwise
// network_listen_reject_pending_connection is used -- types/networking.h currently leaves this
// field undocumented ("never read or written by the module" does NOT appear for it, but no
// specific purpose is recorded either); this rewrite casts it to a callback type locally rather
// than asserting a header change.
// UNSURE: network_server_validate_join_request (outside this batch's range) is called with no visible argument; supplied
// the listening endpoint as a guess.
// UNSURE: the loopback/local-address check that sets a new child `connected` uses
// network_channel_get_remote_address's queue->last_error side channel exactly like
// network_channel_remote_address_or_default.c, reconstructed the same way (see that file for the general
// pattern) rather than calling the still-unresolved network_channel_remote_address_or_default shared guard, since what is
// needed here is the resolved address itself, which the guard never exposes.
// register/parameter convention: channel in ECX/param_1 (Ghidra recognizes it as a real stack
// parameter here), out-new-child in param_2.
```

```
#if 0
Original Ghidra decompilation (0x4dd4e0):

char FUN_004dd4e0(int *param_1,int *param_2)

{
  int iVar1;
  int *piVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int *piVar6;
  int local_18;

  piVar2 = param_1;
  *param_2 = 0;
  param_1._0_1_ = '\x01';
  sVar3 = FUN_004419d0();
  if (sVar3 != 0) {
    if (sVar3 == -0xd) {
      return '\x01';
    }
    return '\0';
  }
  *(undefined4 *)(piVar2[0x2a7] + 0x110) = 0;
  sVar3 = 0;
LAB_004dd521:
  do {
    iVar5 = piVar2[0x2a7];
    iVar4 = *(int *)(iVar5 + 0x110);
    if (*(int *)(iVar5 + 0x10c) < iVar4) {
      return (char)param_1;
    }
    iVar1 = *(int *)(*(int *)(iVar5 + 0x104) + iVar4 * 4);
    *(int *)(iVar5 + 0x110) = iVar4 + 1;
    if (iVar1 == 0) {
      return (char)param_1;
    }
    if (sVar3 != 0) {
      return (char)param_1;
    }
    if ((*(char *)(iVar1 + 4) == '\x01') && (0 < DAT_006f16d0)) {
LAB_004dd582:
      if (iVar1 == *piVar2) {
        if (*(int *)(piVar2[0x2a7] + 0x10c) + 1 < 0x11) {
          if ((char)piVar2[0x2b8] == '\0') {
            iVar5 = 2;
          }
          else {
            iVar5 = FUN_004e0850();
            if (iVar5 == 0) {
              iVar5 = network_listen_accept_pending_connection();
              if ((iVar5 != 0) && (iVar5 = FUN_004dd430(iVar5), iVar5 != 0)) {
                iVar4 = 0;
                piVar6 = piVar2 + 0x2a8;
                do {
                  if (*piVar6 == 0) {
                    *param_2 = iVar5;
                    piVar2[iVar4 + 0x2a8] = iVar5;
                    *(int **)(iVar5 + 0xa94) = piVar2;
                    *(undefined1 *)(piVar2[iVar4 + 0x2a8] + 0xa98) = 0;
                    if ((*(char *)(*(int *)(piVar2[iVar4 + 0x2a8] + 0xa94) + 0xae1) == '\0') &&
                       ((FUN_004dd390(), local_18 == 0x7f000001 || (local_18 == DAT_006869b0)))) {
                      *(undefined1 *)(piVar2[iVar4 + 0x2a8] + 0xa98) = 1;
                      *(undefined1 *)(*(int *)(piVar2[iVar4 + 0x2a8] + 0xa94) + 0xae1) = 1;
                    }
                    break;
                  }
                  iVar4 = iVar4 + 1;
                  piVar6 = piVar6 + 1;
                } while (iVar4 < 0x10);
              }
              goto LAB_004dd5e2;
            }
          }
        }
        else {
          iVar5 = 6;
        }
        if ((code *)piVar2[2] == (code *)0x0) {
          sVar3 = FUN_00442250(iVar5);
        }
        else {
          (*(code *)piVar2[2])(iVar1);
        }
      }
      else {
        iVar5 = 0;
        piVar6 = piVar2 + 0x2a8;
        do {
          if (((int *)*piVar6 != (int *)0x0) && (*(int *)*piVar6 == iVar1)) {
            param_1._0_1_ = network_channel_transmit((int *)piVar2[iVar5 + 0x2a8]);
            if ((char)param_1 == '\0') {
              network_channel_list_remove(*(undefined4 *)piVar2[iVar5 + 0x2a8]);
              *(uint *)(piVar2[iVar5 + 0x2a8] + 0xa8c) =
                   *(uint *)(piVar2[iVar5 + 0x2a8] + 0xa8c) | 0x10;
              param_1._0_1_ = '\x01';
            }
            goto LAB_004dd521;
          }
          iVar5 = iVar5 + 1;
          piVar6 = piVar6 + 1;
        } while (iVar5 < 0x10);
      }
    }
    else {
      iVar5 = *(int *)(iVar1 + 0x10);
      if (iVar5 != 0) {
        iVar4 = *(int *)(iVar5 + 0xc) - *(int *)(iVar5 + 8);
        if (iVar4 < 0) {
          iVar4 = iVar4 + *(int *)(iVar5 + 0x10);
        }
        if (0 < iVar4) goto LAB_004dd582;
      }
    }
LAB_004dd5e2:
    if ((char)param_1 == '\0') {
      return '\0';
    }
  } while( true );
}
#endif
```

## network_channel_new.c

```
// network_channel_new  (Ghidra: network_channel_new, already named)
// address 0x4dc9b0, size 295 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Allocates and initializes a new network
// transport/channel object (socket, incoming buffer, bookkeeping) sized and configured according
// to the requested flag bits; frees everything and returns NULL on failure." Every dword-indexed
// offset Ghidra shows (channel[0x2b8]=0xae0, channel+0xae1, channel[0x2a7]=0xa9c,
// channel[0x2a3]=0xa8c, channel[0]=0x000, channel[3]=0x00c, channel[0x29e..0x2a2]=0xa78..0xa88)
// matches types/networking.h's network_channel field offsets (listening, child_busy,
// listen_list, flags, endpoint, incoming, reliable_count, reliable, send_budget,
// budget_base_tick, rate_index) exactly, for both the k_network_channel_listening (0xae4-byte)
// and plain (0xa9c-byte) allocation sizes.
// UNSURE: network_channel_stream_init is called twice with no visible argument; reconstructed as
// (&channel->outgoing) then (&channel->retransmit), matching its own summary ("initializes a small
// per-direction bookkeeping record ... for a newly-created channel") and the two
// network_channel_stream sub-objects a channel owns.
// UNSURE: circular_buffer_new's own file declares it void (its pointer result is read back via
// the implicit EAX-return convention that file's header already documents); the extern below
// intentionally redeclares it returning a pointer to match how every caller must use it.
```

```
#if 0
Original Ghidra decompilation (0x4dc9b0):

int * __cdecl network_channel_new(uint flags)

{
  bool bVar1;
  short sVar2;
  int *channel;
  int iVar3;
  void *pvVar4;
  DWORD DVar5;

  bVar1 = true;
  if ((flags & 1) == 0) {
    if ((flags & 2) == 0) {
      return (int *)0x0;
    }
    channel = GlobalAlloc(0x40,0xa9c);
    if (channel == (int *)0x0) {
      return (int *)0x0;
    }
  }
  else {
    channel = GlobalAlloc(0x40,0xae4);
    if (channel == (int *)0x0) {
      return (int *)0x0;
    }
    *(undefined1 *)(channel + 0x2b8) = 1;
    *(undefined1 *)((int)channel + 0xae1) = 0;
    iVar3 = FUN_00441960();
    channel[0x2a7] = iVar3;
    if (iVar3 == 0) {
      network_channel_delete(channel);
      return (int *)0x0;
    }
  }
  network_channel_record_timestamp((int)channel);
  channel[0x2a3] = flags;
  pvVar4 = network_receive_queue_new();
  *channel = (int)pvVar4;
  if ((pvVar4 != (void *)0x0) &&
     (((flags & 1) == 0 ||
      ((sVar2 = network_listen_start(), sVar2 == 0 &&
       (sVar2 = network_channel_list_add(), sVar2 == 0)))))) {
    iVar3 = circular_buffer_new("transport-incoming");
    channel[3] = iVar3;
    if (iVar3 != 0) goto LAB_004dca7f;
  }
  bVar1 = false;
LAB_004dca7f:
  channel[0x29e] = 0;
  channel[0x29f] = 0;
  channel[0x2a0] = 0xe0;
  channel[0x2a2] = 0;
  DVar5 = GetTickCount();
  channel[0x2a1] = DVar5;
  FUN_004dd980();
  FUN_004dd980();
  if (bVar1) {
    return channel;
  }
  network_channel_delete(channel);
  return (int *)0x0;
}
#endif
```

## network_channel_new_child.c

```
// network_channel_new_child  (Ghidra: FUN_004dd430; named per this rewrite)
// address 0x4dd430, size 172 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Allocates and initializes a new child network
// channel object for a freshly-accepted incoming connection, mirroring the smaller-variant setup
// done by the general channel constructor." Same field offsets as network_channel_new.c's plain
// (0xa9c-byte) allocation, with flags hard-coded to k_network_channel_transmit_pending (4)
// instead of being a parameter.
```

```
#if 0
Original Ghidra decompilation (0x4dd430):

int * FUN_004dd430(int param_1)

{
  int *channel;
  int iVar1;
  DWORD DVar2;

  channel = GlobalAlloc(0x40,0xa9c);
  if (channel != (int *)0x0) {
    *channel = param_1;
    channel[0x2a3] = 4;
    iVar1 = circular_buffer_new("transport-incoming");
    channel[3] = iVar1;
    network_channel_record_timestamp((int)channel);
    channel[0x29e] = 0;
    channel[0x29f] = 0;
    channel[0x2a0] = 0xe0;
    channel[0x2a2] = 0;
    DVar2 = GetTickCount();
    channel[0x2a1] = DVar2;
    FUN_004dd980();
    FUN_004dd980();
    if (iVar1 == 0) {
      network_channel_delete(channel);
      return (int *)0x0;
    }
  }
  return channel;
}
#endif
```

## network_channel_queue_message.c

```
// network_channel_queue_message  (Ghidra: FUN_004dce40; named per this rewrite)
// address 0x4dce40, size 194 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Queues an outgoing message either for
// immediate transmission or, when not marked immediate, into the reliable retransmission buffer
// pool, depending on a caller-supplied mode flag." Matches: when immediate (param_4 == 1) and
// there is room, writes header then body bits directly into a channel bit_stream via
// bit_stream_write_bits_chunked and marks it non-empty; otherwise (or if there is no room even
// after one flush attempt via network_channel_stream_flush) falls back to
// network_channel_reliable_pool_store (network_channel_reliable_pool_store).
// UNSURE (significant): the free-space formula this function computes
// (last_bit - byte_cursor*8 - bit_cursor + 1) reads channel+0x1c/+0x20/+0x24, which land on
// types/networking.h's network_channel.in (the struct's own comment calls `in` "the
// receive-side stream" and `out`, at +0x544, "the send-side stream, drained by 0x4dd730"). But
// network_channel_transmit's own decompile (0x4dd730) drains channel->incoming (+0x00c) and
// channel->endpoint, never touching +0x544 at all -- so the header's out/in direction labels are
// not confirmed by that function either. This rewrite follows the concrete offsets (channel->outgoing)
// rather than the header's descriptive comment, which is the more likely source of error.
// UNSURE: param_1/param_2 are declared but never read anywhere in Ghidra's own decompile; this
// rewrite assumes they are the header/body bit VALUES that bit_stream_write_bits_chunked needs
// (register-forwarded past Ghidra's dead-parameter analysis, the same class of issue documented
// throughout this batch) rather than genuinely unused, since no other source for those two
// values exists in this function's visible body.
// register/parameter convention: EBX -> body_bit_count, EDI -> channel (both elided). blam-cc:
// EBX -> body_bit_count, stack -> header_value, body_value, header_bit_count, immediate, flush_after
```

```
#if 0
Original Ghidra decompilation (0x4dce40):

char FUN_004dce40(undefined4 param_1,undefined4 param_2,int param_3,char param_4,char param_5)

{
  char extraout_AL;
  char cVar1;
  int unaff_EBX;
  int unaff_EDI;

  if ((*(byte *)(unaff_EDI + 0xa8c) & 1) != 0) {
    return '\x01';
  }
  cVar1 = '\x01';
  if (param_4 == '\x01') {
    if ((((*(int *)(unaff_EDI + 0x24) + *(int *)(unaff_EDI + 0x1c) * -8) -
         *(int *)(unaff_EDI + 0x20)) + 1 < unaff_EBX + param_3) &&
       (FUN_004ddb60(), cVar1 = extraout_AL, extraout_AL == '\0')) {
      return '\0';
    }
    *(int *)(unaff_EDI + 0xa80) = *(int *)(unaff_EDI + 0xa80) + unaff_EBX + param_3;
    bit_stream_write_bits_chunked(param_3);
    *(undefined1 *)(unaff_EDI + 0x2c) = 0;
    bit_stream_write_bits_chunked();
    *(undefined1 *)(unaff_EDI + 0x2c) = 0;
    if (param_5 != '\x01') {
      return cVar1;
    }
    cVar1 = FUN_004ddb60();
    return cVar1;
  }
  FUN_004dcdb0();
  return '\x01';
}
#endif
```

## network_channel_receive_callback.c

```
// network_channel_receive_callback  (Ghidra: network_channel_receive_callback, already named)
// address 0x441ed0, size 94 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("per-channel data-received handler:
// records the sender's address for connectionless channels, or appends the payload to the
// channel's receive circular buffer for connection-oriented ones"); the tested flag (+0x0c bit
// 0) is network_receive_queue.flags bit0 "connection oriented" per types/networking.h, and the
// registered lookup gt2GetConnectionData(gamespy_handle) -> network_receive_queue* is reused from
// network_connection_stats_record_packet.c, whose evidence note documents the same callee.
// register convention: all three parameters are real (stack/callback) parameters, not
// Ghidra-recognized registers -- this is registered as a callback with a fixed signature by
// the foreign transport library, called with (handle, data, length).
// UNSURE: gt2AddressToString's local 24-byte output buffer is filled and then never read again; kept
// exactly as decompiled rather than assumed dead, since the callee may have an internal side
// effect (e.g. caching the formatted address) that this module does not see. UNSURE: the
// `circular_buffer_write(length, queue->incoming, data)` argument order for the one visible
// call-site parameter (source buffer) is reconstructed from circular_buffer_write's own
// documented register convention (byte count EAX, stream EDX, source as the recognized
// parameter), not shown explicitly at this call site.
```

```
#if 0
Original Ghidra decompilation (0x441ed0):

void network_channel_receive_callback(undefined4 param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined1 local_18 [24];

  iVar1 = FUN_00614840(param_1);
  if (iVar1 != 0) {
    if ((*(byte *)(iVar1 + 0xc) & 1) == 0) {
      uVar2 = FUN_006175f0(param_1);
      uVar3 = FUN_006147d0(param_1);
      FUN_006148b0(uVar2,uVar3,local_18);
    }
    else if (0 < param_3) {
      circular_buffer_write(param_2);
      return;
    }
  }
  return;
}
#endif
```

## network_channel_record_timestamp.c

```
// network_channel_record_timestamp  (Ghidra: network_channel_record_timestamp, already named)
// address 0x4dd930, size 66 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md: "Records the current time (in milliseconds)
// into a channel's timestamp field, used elsewhere for timeout comparisons." channel+4 matches
// types/networking.h's network_channel.last_activity_ms exactly.
```

```
#if 0
Original Ghidra decompilation (0x4dd930):

void __cdecl network_channel_record_timestamp(int channel)

{
  undefined4 uVar1;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(channel + 4) = uVar1;
  return;
}
#endif
```

## network_channel_reliable_pool_ensure_capacity.c

```
// network_channel_reliable_pool_ensure_capacity  (Ghidra: FUN_004dcc30; named per this rewrite)
// address 0x4dcc30, size 367 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Finds (or grows and allocates) a reusable pair
// of fixed-size scratch buffers in the channel's reliable-message buffer pool, sized to hold at
// least the requested header/body byte counts." Fully recovered cdecl parameters; every offset
// (+0xa78, +0xa7c, and the reliable-slot fields +0x00/+0x04/+0x08/+0x0c/+0x10/+0x14/+0x18/+0x1c)
// matches types/networking.h's network_channel.reliable_count/reliable and
// network_channel_reliable_slot exactly. types/networking.h's own comment on this function
// ("the argument order in 0x4dcdb0 is the opposite of the capacity order in 0x4dcc30, which is
// why the capacities are cross-named") confirms param_2 = needed body capacity, param_3 = needed
// header capacity.
// FIXED against objdump -d -M intel bin/halo.exe (0x4dcc30..0x4dcda7): Ghidra types this
// function void, but it is not -- the early-return path (`jne 0x4dcda3` at 0x4dcc7a) jumps
// straight to the epilogue with EAX still holding the matched slot's loop index, and the
// grow path's last `mov eax,[ebp+0xa78]` at 0x4dcd94 loads the OLD reliable_count (the index of
// the first newly-allocated slot) right before the epilogue. Both paths leave a valid slot index
// in EAX, so this rewrite returns int32_t; the caller (network_channel_reliable_pool_store.c)
// uses exactly this index.
// The `if (iVar1 != -1) return;` / `break` pair right after a match is found is unreachable dead
// code (the loop counter is never -1 at that point) and is kept verbatim rather than simplified.
```

```
#if 0
Original Ghidra decompilation (0x4dcc30):

void FUN_004dcc30(int param_1,int param_2,int param_3)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;
  HGLOBAL pvVar4;
  char *pcVar5;
  undefined4 *puVar6;
  int iVar7;
  undefined4 *puVar8;

  iVar1 = 0;
  if (0 < *(int *)(param_1 + 0xa78)) {
    pcVar5 = *(char **)(param_1 + 0xa7c);
    do {
      if (((*pcVar5 == '\0') && (param_2 <= *(int *)(pcVar5 + 0xc))) &&
         (param_3 <= *(int *)(pcVar5 + 8))) {
        if (iVar1 != -1) {
          return;
        }
        break;
      }
      iVar1 = iVar1 + 1;
      pcVar5 = pcVar5 + 0x20;
    } while (iVar1 < *(int *)(param_1 + 0xa78));
  }
  puVar2 = GlobalAlloc(0,(*(int *)(param_1 + 0xa78) + 10) * 0x20);
  if (0 < (int)*(uint *)(param_1 + 0xa78)) {
    puVar6 = *(undefined4 **)(param_1 + 0xa7c);
    puVar8 = puVar2;
    for (iVar1 = (*(uint *)(param_1 + 0xa78) & 0x7ffffff) << 3; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar8 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar8 = puVar8 + 1;
    }
    for (iVar1 = 0; iVar1 != 0; iVar1 = iVar1 + -1) {
      *(undefined1 *)puVar8 = *(undefined1 *)puVar6;
      puVar6 = (undefined4 *)((int)puVar6 + 1);
      puVar8 = (undefined4 *)((int)puVar8 + 1);
    }
    GlobalFree(*(HGLOBAL *)(param_1 + 0xa7c));
  }
  iVar1 = *(int *)(param_1 + 0xa78);
  *(undefined4 **)(param_1 + 0xa7c) = puVar2;
  if (iVar1 < iVar1 + 10) {
    iVar7 = iVar1 << 5;
    do {
      *(undefined1 *)(iVar7 + *(int *)(param_1 + 0xa7c)) = 0;
      *(undefined4 *)(iVar7 + 0x14 + *(int *)(param_1 + 0xa7c)) = 0;
      *(undefined4 *)(iVar7 + 0x10 + *(int *)(param_1 + 0xa7c)) = 0;
      *(undefined4 *)(iVar7 + 4 + *(int *)(param_1 + 0xa7c)) = 0xffffffff;
      iVar3 = param_3;
      if (param_3 < 100) {
        iVar3 = 100;
      }
      *(int *)(iVar7 + 8 + *(int *)(param_1 + 0xa7c)) = iVar3;
      iVar3 = param_2;
      if (param_2 < 100) {
        iVar3 = 100;
      }
      *(int *)(iVar7 + 0xc + *(int *)(param_1 + 0xa7c)) = iVar3;
      pvVar4 = GlobalAlloc(0,*(SIZE_T *)(iVar7 + 0xc + *(int *)(param_1 + 0xa7c)));
      *(HGLOBAL *)(iVar7 + 0x1c + *(int *)(param_1 + 0xa7c)) = pvVar4;
      pvVar4 = GlobalAlloc(0,*(SIZE_T *)(iVar7 + 8 + *(int *)(param_1 + 0xa7c)));
      *(HGLOBAL *)(iVar7 + 0x18 + *(int *)(param_1 + 0xa7c)) = pvVar4;
      iVar1 = iVar1 + 1;
      iVar7 = iVar7 + 0x20;
    } while (iVar1 < *(int *)(param_1 + 0xa78) + 10);
  }
  *(int *)(param_1 + 0xa78) = *(int *)(param_1 + 0xa78) + 10;
  return;
}
#endif
```

## network_channel_reliable_pool_store.c

```
// network_channel_reliable_pool_store  (Ghidra: FUN_004dcdb0; named per this rewrite)
// address 0x4dcdb0, size 142 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Stores a header/body message pair into a
// tagged slot of the channel's reliable-message buffer pool for later (re)transmission
// tracking." Calls network_channel_reliable_pool_ensure_capacity(channel, body_bytes,
// header_bytes) with the byte counts rounded up from bit counts (ceil(bits/8)), matching
// types/networking.h's note that this function's argument order is the reverse of that one's.
// register/parameter convention: EAX -> header_bits, ECX -> body_bits (both elided from
// Ghidra's own signature); stack -> channel, body_data, header_data, priority.
// blam-cc: EAX -> header_bits, ECX -> body_bits, stack -> channel, body_data, header_data, priority
```

```
#if 0
Original Ghidra decompilation (0x4dcdb0):

void FUN_004dcdb0(int param_1,undefined4 *param_2,undefined4 *param_3,undefined4 param_4)

{
  uint in_EAX;
  uint uVar1;
  int iVar2;
  undefined1 *puVar3;
  uint in_ECX;
  uint uVar4;
  uint uVar5;
  undefined4 *puVar6;

  uVar1 = (uint)((in_EAX & 7) != 0) + (in_EAX >> 3);
  uVar5 = (uint)((in_ECX & 7) != 0) + (in_ECX >> 3);
  iVar2 = FUN_004dcc30(param_1,uVar5,uVar1);
  puVar3 = (undefined1 *)(iVar2 * 0x20 + *(int *)(param_1 + 0xa7c));
  *(undefined4 *)(puVar3 + 4) = param_4;
  *(uint *)(puVar3 + 0x10) = in_EAX;
  *(uint *)(puVar3 + 0x14) = in_ECX;
  *puVar3 = 1;
  puVar6 = *(undefined4 **)(puVar3 + 0x18);
  for (uVar4 = uVar1 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *puVar6 = *param_3;
    param_3 = param_3 + 1;
    puVar6 = puVar6 + 1;
  }
  for (uVar1 = uVar1 & 3; uVar1 != 0; uVar1 = uVar1 - 1) {
    *(undefined1 *)puVar6 = *(undefined1 *)param_3;
    param_3 = (undefined4 *)((int)param_3 + 1);
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  puVar6 = *(undefined4 **)(puVar3 + 0x1c);
  for (uVar1 = uVar5 >> 2; uVar1 != 0; uVar1 = uVar1 - 1) {
    *puVar6 = *param_2;
    param_2 = param_2 + 1;
    puVar6 = puVar6 + 1;
  }
  for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined1 *)puVar6 = *(undefined1 *)param_2;
    param_2 = (undefined4 *)((int)param_2 + 1);
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  return;
}
#endif
```

## network_channel_remote_address_or_default.c

```
// network_channel_remote_address_or_default  (Ghidra: FUN_004dd390; renamed by the review pass
// from the rewriters' network_message_decode_guard)
// address 0x4dd390, size 84 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: the disassembly settles what the decompilation could not. 0x4dd390 is
//     push esi / mov esi,ecx / test esi,esi / je ret        -> ECX is the out record, may be NULL
//     mov edi,[eax] / test edi,edi / je default             -> EAX is a network_channel *, and
//                                                              [eax] is channel->endpoint
//     call 0x441ce0                                         -> network_channel_get_remote_address
//                                                              with ESI = out record, EDI = queue
//     test ax,ax / je ret-unchanged                         -> 0x441ce0 returns its error code in
//                                                              AX (0, or 0xfff1 on the no-address
//                                                              path at 0x441de1), NOT void
//     zero six dwords at [esi] / mov WORD [esi+0x10],4      -> the default is 0.0.0.0:0 with
//                                                              size = k_network_address_size_ipv4
// So this function resolves the remote address of `channel` into `out_address`, substituting a
// zeroed IPv4 address when the channel has no endpoint or the transport cannot report one.
// The rewriters modeled the record as an opaque "decode result" with a `state` field; the
// WORD 4 at +0x10 is s_network_address::size and the dword at +0x00 that every decode handler
// compares is s_network_address::ipv4 -- the handlers are checking the sender against the peer
// they expect. Renamed and retyped accordingly; see types/networking.h's network_resolved_address.
// Call sites confirming the EAX argument is the channel itself, not a slot holding it:
//   0x4dbcc0  mov eax,[esi+0xadc]        (client->channel)
//   0x4dc090  mov eax,[esi+0xadc]        (client->channel)
//   0x4d9f1d  mov eax,[esi+0xadc]        (client->channel), ecx = client+0xef8
//   0x4df690  mov eax,[esi]              (machine->channel)
//   0x4defd4  mov eax,edi                (the freshly accepted child channel)
// register convention: channel in EAX, out_address in ECX. blam-cc: EAX -> channel, ECX -> out_address
```

```
#if 0
Original Ghidra decompilation (0x4dd390):

void FUN_004dd390(void)

{
  undefined2 uVar1;
  int in_EAX;
  undefined4 *in_ECX;
  int unaff_EDI;

  if (in_ECX != (undefined4 *)0x0) {
    if (*(int *)in_EAX == 0) {
      *in_ECX = 0;
      in_ECX[1] = 0;
      in_ECX[2] = 0;
      in_ECX[3] = 0;
      in_ECX[4] = 0;
      in_ECX[5] = 0;
      *(undefined2 *)((int)in_ECX + 0x10) = 4;
    }
    else {
      uVar1 = network_channel_get_remote_address();
      if (uVar1 != 0) {
        *in_ECX = 0;
        in_ECX[1] = 0;
        in_ECX[2] = 0;
        in_ECX[3] = 0;
        in_ECX[4] = 0;
        in_ECX[5] = 0;
        *(undefined2 *)((int)in_ECX + 0x10) = 4;
      }
    }
  }
  return;
}
#endif
```

## network_channel_remove_child.c

```
// network_channel_remove_child  (Ghidra: FUN_004dd090; named per this rewrite)
// address 0x4dd090, size 123 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Finds a specific child channel in the parent's
// child table, closes its socket, deletes the child object, and clears the table entry."
// children[] (0xaa0, 16 entries) and connected (0xa98) match types/networking.h's
// network_channel exactly.
// UNSURE: network_channel_list_remove's second argument (the list to remove from) is elided at
// this call site; reconstructed as parent->listen_list, matching the equivalent call in
// network_channel_delete.c.
// register convention: parent channel in EDI (unaff_EDI). blam-cc: EDI -> parent, stack -> child
```

```
#if 0
Original Ghidra decompilation (0x4dd090):

undefined4 FUN_004dd090(int *param_1)

{
  int *piVar1;
  int iVar2;
  int unaff_EDI;

  iVar2 = 0;
  piVar1 = (int *)(unaff_EDI + 0xaa0);
  while (((int *)*piVar1 == (int *)0x0 || ((int *)*piVar1 != param_1))) {
    iVar2 = iVar2 + 1;
    piVar1 = piVar1 + 1;
    if (0x10 < iVar2) {
      return 0;
    }
  }
  if (*param_1 != 0) {
    network_channel_list_remove(**(undefined4 **)(unaff_EDI + 0xaa0 + iVar2 * 4));
  }
  if (*(char *)(*(int *)(unaff_EDI + 0xaa0 + iVar2 * 4) + 0xa98) == '\x01') {
    *(undefined1 *)(unaff_EDI + 0xae1) = 0;
  }
  network_channel_delete(*(int **)(unaff_EDI + 0xaa0 + iVar2 * 4));
  *(undefined4 *)(unaff_EDI + 0xaa0 + iVar2 * 4) = 0;
  return 1;
}
#endif
```

## network_channel_scan_retransmit_timeouts.c

```
// network_channel_scan_retransmit_timeouts  (Ghidra: network_channel_scan_retransmit_timeouts,
// already named)
// address 0x4dd9d0, size 368 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Scans the channel's reliable-message buffer
// pool for entries that are overdue based on an estimated delivery rate and retransmits them,
// then clears the pool's active flags." The free-space formula
// (out.stream.last_bit - out.stream.byte_cursor*8 - out.stream.bit_cursor + 1) matches
// types/networking.h's own comment on this exact function almost verbatim, confirming `out`
// (not `in`) is the stream actually used here -- see network_channel_queue_message.c's header
// for the discrepancy with that other function.
// UNSURE: bit_stream_write_bits_chunked's `value` argument is elided at both call sites here;
// this rewrite reads it as a raw 32-bit load from slot->header / slot->body, which is only
// correct for messages whose header/body each fit in 4 bytes -- the same simplification
// network_channel_queue_message.c already documents for the encode side.
// register/parameter convention: fully recovered cdecl (channel is the only parameter).
```

```
#if 0
Original Ghidra decompilation (0x4dd9d0):

void __cdecl network_channel_scan_retransmit_timeouts(int channel)

{
  int iVar1;
  char cVar2;
  DWORD DVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int local_c;
  int local_8;

  iVar1 = channel;
  DVar3 = GetTickCount();
  iVar4 = *(int *)(channel + 0xa84);
  iVar5 = DAT_00710308;
  if (DAT_00710308 == 0) {
    iVar5 = *(int *)(&DAT_00697edc + *(int *)(channel + 0xa88) * 4);
  }
  local_8 = 0;
  do {
    local_c = 0;
    if (0 < *(int *)(iVar1 + 0xa78)) {
      channel = 0;
      do {
        iVar8 = *(int *)(iVar1 + 0xa7c) + channel;
        if (((*(char *)(*(int *)(iVar1 + 0xa7c) + channel) == '\x01') &&
            (*(int *)(iVar8 + 4) == local_8)) &&
           (iVar7 = *(int *)(iVar8 + 0x14) + *(int *)(iVar8 + 0x10),
           iVar6 = (iVar5 / 1000) * (DVar3 - iVar4) - *(int *)(iVar1 + 0xa80),
           iVar6 != iVar7 && -1 < iVar6 - iVar7)) {
          if ((iVar7 <= ((*(int *)(iVar1 + 0x558) + *(int *)(iVar1 + 0x550) * -8) -
                        *(int *)(iVar1 + 0x554)) + 1) ||
             (cVar2 = FUN_004ddb60(iVar1,0), cVar2 != '\0')) {
            bit_stream_write_bits_chunked(*(undefined4 *)(iVar8 + 0x10));
            *(undefined1 *)(iVar1 + 0x560) = 0;
            bit_stream_write_bits_chunked(*(undefined4 *)(iVar8 + 0x14));
            *(undefined1 *)(iVar1 + 0x560) = 0;
          }
          *(int *)(iVar1 + 0xa80) = *(int *)(iVar1 + 0xa80) + iVar7;
        }
        local_c = local_c + 1;
        channel = channel + 0x20;
      } while (local_c < *(int *)(iVar1 + 0xa78));
    }
    local_8 = local_8 + 1;
  } while (local_8 < 10);
  iVar4 = 0;
  if (0 < *(int *)(iVar1 + 0xa78)) {
    iVar5 = 0;
    do {
      *(undefined1 *)(iVar5 + *(int *)(iVar1 + 0xa7c)) = 0;
      iVar4 = iVar4 + 1;
      iVar5 = iVar5 + 0x20;
    } while (iVar4 < *(int *)(iVar1 + 0xa78));
  }
  return;
}
#endif
```

## network_channel_service.c

```
// network_channel_service  (Ghidra: FUN_004dd110; named per this rewrite)
// address 0x4dd110, size 292 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Per-tick service routine for a network
// channel: updates timing/status flags, scans for and retransmits timed-out reliable messages,
// and dispatches to the receive or transmit path as needed." channel[1]=+0x004
// (last_activity_ms), channel[0x2a3]=+0xa8c (flags), channel[0xb]=+0x02c (in.empty),
// channel[0x158]=+0x560 (out.empty), channel[0x2a0]=+0xa80 (send_budget) all match
// types/networking.h's network_channel exactly.
// `timeout_ms` (EAX, kept in ESI): idle timeout added to last_activity_ms; 0 skips the timing block.
// register convention: timeout_ms in EAX (in_EAX), channel in EDI (unaff_EDI), plus ONE real
// cdecl stack argument that Ghidra dropped entirely: 0x4dd202 is `mov edx,[esp+0x18]` (the
// first stack parameter, since the frame is sub esp,8 + three pushes) and it is forwarded
// straight to network_channel_listen_service as its out-new-child pointer. Both in-module
// callers (0x4daef0, 0x4db100) push 0 for it.
// blam-cc: EAX -> timeout_ms, EDI -> channel, stack -> out_new_child

// VERIFIED against disassembly 0x4dd110..0x4dd234 (2026-09-30): now via QPC*1000/freq (_allmul/_alldiv); timeout_ms in EAX = idle timeout; flags/timeout/backoff branches, flush modes 1/0, listen vs transmit dispatch match
```

```
#if 0
Original Ghidra decompilation (0x4dd110):

char FUN_004dd110(void)

{
  uint uVar1;
  char cVar2;
  int in_EAX;
  uint uVar3;
  int *unaff_EDI;
  undefined8 uVar4;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar4 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar3 = __alldiv(uVar4,DAT_006ac8f8,DAT_006ac8fc);
  uVar1 = unaff_EDI[0x2a3];
  unaff_EDI[0x2a3] = uVar1 & 0xffffffdf;
  if (in_EAX != 0) {
    if (unaff_EDI[1] + 5000U < uVar3) {
      unaff_EDI[0x2a3] = uVar1 & 0xffffffdf | 0x20;
    }
    if (uVar3 <= (uint)(unaff_EDI[1] + in_EAX)) goto LAB_004dd1b5;
    if ((DAT_0071c2c8 == '\0') &&
       ((DAT_00697ed8 * 0x1e < *(int *)(DAT_006f1d6c + 0xc) || (DAT_00719720 == 1)))) {
      return '\0';
    }
  }
  unaff_EDI[1] = uVar3;
LAB_004dd1b5:
  network_channel_scan_retransmit_timeouts((int)unaff_EDI);
  if ((char)unaff_EDI[0xb] == '\0') {
    FUN_004ddb60();
  }
  if ((char)unaff_EDI[0x158] == '\0') {
    FUN_004ddb60();
  }
  unaff_EDI[0x2a0] = 0xe0;
  if ((unaff_EDI[0x2a3] & 1U) != 0) {
    cVar2 = FUN_004dd4e0();
    return cVar2;
  }
  if ((unaff_EDI[0x2a3] & 6U) != 0) {
    cVar2 = network_channel_transmit(unaff_EDI);
    return cVar2;
  }
  return '\x01';
}
#endif
```

## network_channel_service_close_if_disconnected.c

```
// network_channel_service_close_if_disconnected  (Ghidra: FUN_004dd3f0; named per this rewrite)
// address 0x4dd3f0, size 60 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "If the channel is in an active
// connected/connecting state, flushes pending output and then performs a follow-up
// teardown/reconnect step." channel->flags (k_network_channel_client /
// k_network_channel_transmit_pending), channel->endpoint and network_receive_queue.flags bit0
// (k... connection-oriented) all match types/networking.h.
// register convention: channel in EAX (in_EAX). blam-cc: EAX -> channel
```

```
#if 0
Original Ghidra decompilation (0x4dd3f0):

undefined4 FUN_004dd3f0(void)

{
  uint uVar1;
  int *in_EAX;

  uVar1 = in_EAX[0x2a3];
  if (((((uVar1 & 2) != 0) || ((uVar1 & 4) != 0)) && (*in_EAX != 0)) &&
     ((*(byte *)(*in_EAX + 0xc) & 1) != 0)) {
    if (((uVar1 & 2) != 0) || ((uVar1 & 4) != 0)) {
      network_channel_transmit(in_EAX);
    }
    FUN_00442040();
  }
  return 1;
}
#endif
```

## network_channel_service_light.c

```
// network_channel_service_light  (Ghidra: FUN_004dd240; named per this rewrite)
// address 0x4dd240, size 225 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "A lighter-weight per-tick channel service
// routine, updating timing/status flags and dispatching to receive/transmit without the
// retransmit-timeout scan performed by the full channel service routine." Identical to
// network_channel_service.c (0x4dd110) except it omits the
// network_channel_scan_retransmit_timeouts call and the two network_channel_stream_flush flush calls; see that
// file for the field/offset evidence, which is identical here.
// register convention: timeout_ms in EAX (in_EAX), channel in ESI (unaff_ESI). blam-cc:
// EAX -> timeout_ms, ESI -> channel
```

```
#if 0
Original Ghidra decompilation (0x4dd240):

char FUN_004dd240(void)

{
  uint uVar1;
  char cVar2;
  int in_EAX;
  uint uVar3;
  int *unaff_ESI;
  undefined8 uVar4;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar4 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar3 = __alldiv(uVar4,DAT_006ac8f8,DAT_006ac8fc);
  uVar1 = unaff_ESI[0x2a3];
  unaff_ESI[0x2a3] = uVar1 & 0xffffffdf;
  if (in_EAX != 0) {
    if (unaff_ESI[1] + 5000U < uVar3) {
      unaff_ESI[0x2a3] = uVar1 & 0xffffffdf | 0x20;
    }
    if (uVar3 <= (uint)(unaff_ESI[1] + in_EAX)) goto LAB_004dd2e5;
    if ((DAT_0071c2c8 == '\0') &&
       ((DAT_00697ed8 * 0x1e < *(int *)(DAT_006f1d6c + 0xc) || (DAT_00719720 == 1)))) {
      return '\0';
    }
  }
  unaff_ESI[1] = uVar3;
LAB_004dd2e5:
  if ((unaff_ESI[0x2a3] & 1U) != 0) {
    cVar2 = FUN_004dd4e0();
    return cVar2;
  }
  if ((unaff_ESI[0x2a3] & 6U) != 0) {
    cVar2 = network_channel_transmit(unaff_ESI);
    return cVar2;
  }
  return '\x01';
}
#endif
```

## network_channel_service_retransmit_only.c

```
// network_channel_service_retransmit_only  (Ghidra: FUN_004dd330; named per this rewrite)
// address 0x4dd330, size 89 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Runs the retransmit-timeout scan and
// pending-flush bookkeeping for a channel without performing an actual receive or transmit
// pass." Same in.empty/out.empty/send_budget fields as network_channel_service.c. The Ghidra
// decompile's CONCAT31(uVar1,1) return packs garbage high bytes from extraout_EAX/_var with a
// fixed low byte of 1; this rewrite just returns 1.
// register convention: channel in EDI (unaff_EDI). blam-cc: EDI -> channel
```

```
#if 0
Original Ghidra decompilation (0x4dd330):

undefined4 FUN_004dd330(void)

{
  undefined4 extraout_EAX;
  undefined3 uVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  int unaff_EDI;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  network_channel_scan_retransmit_timeouts(unaff_EDI);
  uVar1 = (undefined3)((uint)extraout_EAX >> 8);
  if (*(char *)(unaff_EDI + 0x2c) == '\0') {
    FUN_004ddb60();
    uVar1 = extraout_var;
  }
  if (*(char *)(unaff_EDI + 0x560) == '\0') {
    FUN_004ddb60();
    uVar1 = extraout_var_00;
  }
  *(undefined4 *)(unaff_EDI + 0xa80) = 0xe0;
  return CONCAT31(uVar1,1);
}
#endif
```

## network_channel_short_disconnect_timeout.c

```
// network_channel_short_disconnect_timeout  (Ghidra: FUN_004ddd20; named per this rewrite)
// address 0x4ddd20, size 27 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md: "Returns true when the host globals are present
// and the DAT_0071c2dc flag is clear; used by the connection state machine to choose a shorter
// disconnect timeout." network_server and network_disconnect_timeout_flag (0x0071c2dc, per
// types/networking.h's own name for this address) match exactly.
```

```
#if 0
Original Ghidra decompilation (0x4ddd20):

undefined4 FUN_004ddd20(void)

{
  if ((DAT_0071c2d4 != 0) && (DAT_0071c2dc == '\0')) {
    return 1;
  }
  return 0;
}
#endif
```

## network_channel_stream_flush.c

```
// network_channel_stream_flush  (Ghidra: FUN_004ddb60; named per this rewrite)
// address 0x4ddb60, size 318 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Register-based (ESI=connection object) helper
// that computes an unsent byte length from a bit-position ring buffer, flushes it through
// gt2Send (a socket/channel send), then advances the connection." The `stream+8/+0xc/+0x10/
// +0x14` fields match bit_stream's first_bit/byte_cursor/bit_cursor/last_bit, and `stream+0x1c`/
// `stream+0x1d` match network_channel_stream's empty flag and inline data buffer exactly, so ESI
// is a network_channel_stream* -- one of channel->in or channel->out depending on the caller
// (unresolvable from this function's own body; see each caller for which one it intends).
// channel->send_budget (+0xa80) and budget_base_tick (+0xa84) match types/networking.h.
// UNSURE: gt2Send's own signature is inferred purely from this call site: (socket,
// buffer, byte_count, mode). `local_c[2]`/the final `0 < local_c[2]` return is preserved as a
// success flag set only when gt2Send reports a positive byte count.
// FIXED: Ghidra's decompile never captures gt2Send's return value (`iVar3`, the loop's own
// exit condition, is left literally unmodified inside the loop body), which would make the
// `while (iVar3 == -4)` retry either never repeat or loop forever depending on the initial byte
// count -- clearly not the intended "retry on a transient socket error" behavior the summary
// describes. This rewrite assigns gt2Send's result back to `byte_count` each iteration,
// which is the only reading that makes the loop terminate sensibly.
// register/parameter convention: ESI -> stream (elided). blam-cc: ESI -> stream, stack ->
// channel, mode
// NOTE for future reconciliation: several already-written files outside this batch's assigned
// range (network_connection_finalize_join.c, network_game_record_message_send.c,
// network_session_info_packet_send.c, network_staged_message_commit.c, and others) declare this
// function as `extern char FUN_004ddb60(network_channel *channel, int32_t unknown)` -- a 2-arg
// guess made before this address was analyzed in depth ("not in this batch"). That guess omits
// the stream argument this rewrite's own analysis of the function body shows is required (ESI
// selects channel->in vs channel->retransmit); those callers' behavior for whichever stream they
// implicitly intended was not re-verified here and their extern declarations were not changed,
// per the rule against editing files outside this batch's assigned range.
```

```
#if 0
Original Ghidra decompilation (0x4ddb60):

bool FUN_004ddb60(int *param_1,char param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  DWORD DVar6;
  int unaff_ESI;
  uint uVar7;
  int local_c [3];

  iVar2 = DAT_0071c2cc;
  uVar1 = *(uint *)(unaff_ESI + 8);
  uVar4 = (*(int *)(unaff_ESI + 0x10) + *(int *)(unaff_ESI + 0xc) * 8) - uVar1;
  uVar7 = uVar4 & 0x80000007;
  local_c[2] = 0;
  if ((int)uVar7 < 0) {
    uVar7 = (uVar7 - 1 | 0xfffffff8) + 1;
  }
  local_c[0] = (uint)(uVar7 != 0) + ((int)(uVar4 + ((int)uVar4 >> 0x1f & 7U)) >> 3);
  if ((uVar1 <= *(uint *)(unaff_ESI + 0x14)) || (uVar1 == *(uint *)(unaff_ESI + 0x14) + 1)) {
    *(uint *)(unaff_ESI + 0xc) = uVar1 >> 3;
    *(uint *)(unaff_ESI + 0x10) = uVar1 & 7;
    iVar5 = bit_stream_write_bits_chunked(iVar2);
    iVar3 = local_c[0];
    if (iVar5 == iVar2) {
      do {
        if (*(char *)(*param_1 + 5) == '\x01') break;
        local_c[0] = 0;
        local_c[1] = 1;
        FUN_006146b0(*(undefined4 *)*param_1,unaff_ESI + 0x1d,iVar3,local_c[param_2 != '\0']);
        if (0 < iVar3) {
          local_c[2] = 1;
          break;
        }
      } while (iVar3 == -4);
    }
  }
  uVar1 = *(uint *)(unaff_ESI + 8);
  local_c[0] = 0;
  *(undefined1 *)(unaff_ESI + 0x1c) = 1;
  if ((uVar1 <= *(uint *)(unaff_ESI + 0x14)) || (uVar1 == *(uint *)(unaff_ESI + 0x14) + 1)) {
    *(uint *)(unaff_ESI + 0x10) = uVar1 & 7;
    *(uint *)(unaff_ESI + 0xc) = uVar1 >> 3;
  }
  bit_stream_write_bits_chunked(DAT_0071c2cc);
  param_1[0x2a0] = param_1[0x2a0] + 0xe0;
  DVar6 = GetTickCount();
  param_1[0x2a1] = DVar6;
  return 0 < local_c[2];
}
#endif
```

## network_channel_stream_init.c

```
// network_channel_stream_init  (Ghidra: FUN_004dd980; named per this rewrite)
// address 0x4dd980, size 80 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Initializes a small per-direction bookkeeping
// record (window sizes, flags) for a newly-created channel and primes the shared output queue
// with a default byte count." Every field written (data = this+0x1d, last_bit = 0x287f,
// capacity_bits = 0x2880, empty = 1) matches types/networking.h's network_channel_stream and
// its k_network_channel_stream_bits constant exactly; network_bit_chunk_size defaults to 11 (0xb)
// here, matching the header's own note.
// UNSURE: bit_stream_write_bits_chunked's value/stream arguments are elided at this call site;
// reconstructed as (0, &stream->stream) -- priming the stream with one chunk-sized zero value --
// matching the summary's "primes ... with a default byte count".
// register convention: stream in EAX (in_EAX). blam-cc: EAX -> stream
```

```
#if 0
Original Ghidra decompilation (0x4dd980):

void FUN_004dd980(void)

{
  int iVar1;
  undefined4 *in_EAX;

  if (DAT_0071c2cc == 0) {
    DAT_0071c2cc = 0xb;
  }
  iVar1 = DAT_0071c2cc;
  *in_EAX = 0;
  in_EAX[1] = (int)in_EAX + 0x1d;
  in_EAX[2] = 0;
  in_EAX[5] = 0x287f;
  in_EAX[6] = 0x2880;
  *(undefined1 *)(in_EAX + 7) = 1;
  in_EAX[4] = 0;
  in_EAX[3] = 0;
  bit_stream_write_bits_chunked(iVar1);
  return;
}
#endif
```

## network_channel_transmit.c

```
// network_channel_transmit  (Ghidra: network_channel_transmit, already named)
// address 0x4dd730, size 504 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md ("pumps queued bytes ... in bounded-size chunks").
// Rewritten against the disassembly 0x4dd730..0x4dd927: the pump drains the transport's receive
// queue (channel->endpoint->incoming, queue +0x10) in chunks of up to 0x5000 bytes into a local
// scratch buffer and appends each chunk to channel->incoming (channel +0x0c) with
// circular_buffer_write (EAX = count, EDX = channel->incoming, stack = scratch). The chunk size
// is limited by the free space in channel->incoming. (The earlier draft had source and
// destination swapped and passed NULL for the remote-address out parameter.)
// register convention: cdecl, channel on the stack; network_channel_get_remote_address takes
// ESI = &local address, EDI = endpoint.
// blam-cc: (channel on the stack)
```

```
#if 0
Original Ghidra decompilation (0x4dd730):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

char __cdecl network_channel_transmit(int *channel)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  undefined8 uVar9;
  char local_5035;
  int local_5034;
  undefined4 *local_502c;
  LARGE_INTEGER local_5028 [4];
  undefined4 local_5008 [5119];
  undefined4 uStack_c;

  uStack_c = 0x4dd740;
  QueryPerformanceCounter(local_5028);
  iVar4 = channel[3];
  iVar1 = *(int *)(iVar4 + 0xc) - *(int *)(iVar4 + 8);
  local_5035 = '\x01';
  if (iVar1 < 0) {
    iVar1 = iVar1 + *(int *)(iVar4 + 0x10);
  }
  iVar1 = *(int *)(iVar4 + 0x10) - iVar1;
  network_channel_get_remote_address();
  do {
    uVar6 = iVar1 - 1;
    iVar4 = *channel;
    if ((*(char *)(iVar4 + 4) != '\x01') || (DAT_006f16d0 < 1)) {
      iVar1 = *(int *)(iVar4 + 0x10);
      if (iVar1 == 0) break;
      iVar2 = *(int *)(iVar1 + 0xc) - *(int *)(iVar1 + 8);
      if (iVar2 < 0) {
        iVar2 = iVar2 + *(int *)(iVar1 + 0x10);
      }
      if (iVar2 < 1) break;
    }
    if ((int)uVar6 < 1) break;
    if (0x4fff < (int)uVar6) {
      uVar6 = 0x5000;
    }
    iVar1 = *(int *)(iVar4 + 0x10);
    uVar3 = *(int *)(iVar1 + 0xc) - *(int *)(iVar1 + 8);
    if ((int)uVar3 < 0) {
      uVar3 = uVar3 + *(int *)(iVar1 + 0x10);
    }
    if (*(char *)(iVar4 + 5) == '\x01') {
      if (uVar3 != 0) goto LAB_004dd7e3;
LAB_004dd8e0:
      channel[0x2a3] = channel[0x2a3] | 0x10;
LAB_004dd8ea:
      local_5035 = '\0';
    }
    else {
      if (uVar3 == 0) break;
LAB_004dd7e3:
      if ((int)uVar6 < (int)uVar3) {
        uVar3 = uVar6;
      }
      local_5034 = *(int *)(iVar1 + 8);
      local_502c = local_5008;
      iVar4 = *(int *)(iVar1 + 0xc) - local_5034;
      if (iVar4 < 0) {
        iVar4 = iVar4 + *(int *)(iVar1 + 0x10);
      }
      if ((int)uVar3 <= iVar4) {
        uVar5 = *(int *)(iVar1 + 0x10) - local_5034;
        uVar6 = uVar3;
        if ((int)uVar5 <= (int)uVar3) {
          puVar7 = (undefined4 *)(*(int *)(iVar1 + 0x14) + local_5034);
          puVar8 = local_5008;
          for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
            *puVar8 = *puVar7;
            puVar7 = puVar7 + 1;
            puVar8 = puVar8 + 1;
          }
          for (uVar6 = uVar5 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
            *(undefined1 *)puVar8 = *(undefined1 *)puVar7;
            puVar7 = (undefined4 *)((int)puVar7 + 1);
            puVar8 = (undefined4 *)((int)puVar8 + 1);
          }
          local_5034 = 0;
          local_502c = (undefined4 *)((int)local_5008 + uVar5);
          uVar6 = uVar3 - uVar5;
        }
        if (0 < (int)uVar6) {
          puVar7 = (undefined4 *)(*(int *)(iVar1 + 0x14) + local_5034);
          for (uVar5 = uVar6 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
            *local_502c = *puVar7;
            puVar7 = puVar7 + 1;
            local_502c = local_502c + 1;
          }
          local_5034 = local_5034 + uVar6;
          for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
            *(undefined1 *)local_502c = *(undefined1 *)puVar7;
            puVar7 = (undefined4 *)((int)puVar7 + 1);
            local_502c = (undefined4 *)((int)local_502c + 1);
          }
        }
        *(int *)(iVar1 + 8) = local_5034;
      }
      if ((int)uVar3 < 1) {
        if (uVar3 != 0xfffffffc) {
          if (uVar3 == 0xfffffffd) goto LAB_004dd8e0;
          goto LAB_004dd8ea;
        }
        break;
      }
      QueryPerformanceCounter(local_5028);
      uVar9 = __allmul(local_5028[0].s.LowPart,local_5028[0].s.HighPart,1000,0);
      iVar4 = __alldiv(uVar9,DAT_006ac8f8,DAT_006ac8fc);
      channel[1] = iVar4;
      circular_buffer_write(local_5008);
    }
    iVar4 = channel[3];
    iVar1 = *(int *)(iVar4 + 0xc) - *(int *)(iVar4 + 8);
    if (iVar1 < 0) {
      iVar1 = iVar1 + *(int *)(iVar4 + 0x10);
    }
    iVar1 = *(int *)(iVar4 + 0x10) - iVar1;
  } while (local_5035 != '\0');
  QueryPerformanceCounter(local_5028);
  return local_5035;
}
#endif
```

## network_channels_close.c

```
// network_channels_close  (Ghidra: network_channels_close, already named)
// address 0x441480, size 57 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: out/phase4/networking_types_notes.md names network_query_socket (0x006f14c8) and
// network_game_socket (0x006f14c4) directly; this is the exact inverse of
// network_channels_open.c, closing both with the same foreign GameSpy transport call.
// register convention: __cdecl, no arguments.
```

```
#if 0
Original Ghidra decompilation (0x441480):

void __cdecl network_channels_close(void)

{
  if (DAT_006f14c8 != 0) {
    FUN_00614860(DAT_006f14c8);
    DAT_006f14c8 = 0;
  }
  if (DAT_006f14c4 != 0) {
    FUN_00614860(DAT_006f14c4);
    DAT_006f14c4 = 0;
  }
  return;
}
#endif
```

## network_channels_open.c

```
// network_channels_open  (Ghidra: network_channels_open, already named)
// address 0x441300, size 375 bytes
// name confidence: 0.55   rewrite confidence: 0.4
// evidence: out/phase4/networking_types_notes.md "network_channel (0xae4 listening / 0xa9c
// plain)" section names network_game_socket (0x006f14c4), network_query_socket (0x006f14c8),
// network_local_address (0x006869b0) and network_channels_open_ok (0x006869be) directly.
// register convention: __cdecl, no arguments.
// UNSURE: every callee here (gt2CreateSocket, gt2SetUnrecognizedMessageCallback/850/8b0, gt2SetSendDump) is
// foreign GameSpy transport library code (see the "server browser" note in
// networking_types_notes.md: those objects are deliberately not declared as Blam types), so
// their real signatures are not recovered -- only the observed argument count/shape is kept.
// UNSURE: LAB_00441020/00441040/00441060/004410b0/00441200 are callback addresses that fall
// in the gap between network_connection_stats_log_tick's end (0x441004) and this function's
// start (0x441300); Ghidra did not lift them as separate functions (they are presumably the
// channel receive/error/accept trampolines wired up here), so they are declared only by
// address and never rewritten in this batch.
// UNSURE: the DAT_006869b0 bit expression is a byte-order swap of network_local_address;
// kept as literal arithmetic rather than assumed to be htonl.
```

```
#if 0
Original Ghidra decompilation (0x441300):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void network_channels_open(void)

{
  int iVar1;
  uint uVar2;
  undefined1 local_30 [24];
  undefined1 local_18 [24];

  uVar2 = (DAT_006869b0 << 0x10 | DAT_006869b0 & 0xff00 | DAT_006869b0 >> 0x10 & 0xff) << 8 |
          DAT_006869b0 >> 0x18;
  DAT_006869be = '\x01';
  FUN_006148b0(uVar2,(undefined2)DAT_00698208,local_18);
  FUN_006148b0(uVar2,_DAT_0069820c & 0xffff,local_30);
  if (DAT_006f14c4 == 0) {
    iVar1 = thunk_FUN_0061c3e0(&DAT_006f14c4,local_18,0,0,&LAB_00441060);
    if (iVar1 == 0) {
      FUN_0061e550(DAT_006f14c4,&LAB_00441020);
      FUN_00614850(DAT_006f14c4,&LAB_00441040);
      FUN_00614800(DAT_006f14c4,&LAB_004410b0);
    }
    else {
      DAT_006869be = '\0';
    }
  }
  if ((DAT_006f14c8 == 0) && (DAT_006869be == '\x01')) {
    iVar1 = thunk_FUN_0061c3e0(&DAT_006f14c8,local_30,0,0,&LAB_00441060);
    if (iVar1 != 0) {
      _DAT_0069820c = 0;
      FUN_006148b0(uVar2,0,local_30);
      iVar1 = thunk_FUN_0061c3e0(&DAT_006f14c8,local_30,0,0,&LAB_00441060);
      if (iVar1 != 0) {
        DAT_006869be = 0;
        return;
      }
    }
    FUN_0061e550(DAT_006f14c8,&LAB_00441020);
    FUN_00614850(DAT_006f14c8,&LAB_00441040);
    FUN_00614800(DAT_006f14c8,&LAB_00441200);
  }
  return;
}
#endif
```

## network_handle_registry_close_all.c

```
// network_handle_registry_close_all  (Ghidra: FUN_00441bb0, still unnamed -> renamed)
// address 0x441bb0, size 59 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("closes and clears any outstanding
// handles left in a fixed-size global handle/flag table"). out/phase2/networking/00.md shows
// the only two call sites: network_receive_queue_new (0x441bf0) calls this before allocating a
// fresh queue, and network_receive_queue_free (0x441c80) calls it right after freeing one --
// both are "sweep anything stale before/after touching the receive-queue pool" call sites.
// The table runs from 0x006f14d0 to (not including) 0x006f16d0, i.e. 64 slots of 8 bytes, and
// ends exactly where network_pending_connection_count (0x006f16d0, types/networking.h) begins,
// which is what pins its length. Each slot's first dword is itself a pointer to an 8-byte
// {handle, in_use-byte} pair -- the exact shape of network_thread_record (types/networking.h)
// -- so the slot is read here as network_thread_record*.
// register convention: __cdecl, no arguments.
// UNSURE: no other function in this module reads or writes this table, so which code
// populates a slot (and therefore what class of thread/handle this registry actually tracks)
// could not be confirmed here; the record shape is reused rather than guessed at.
// network_handle_registry_slot now lives in types/networking.h (folded from this file).
```

```
#if 0
Original Ghidra decompilation (0x441bb0):

void FUN_00441bb0(void)

{
  undefined4 *puVar1;
  undefined4 *puVar2;

  puVar2 = &DAT_006f14d0;
  do {
    puVar1 = (undefined4 *)*puVar2;
    if ((puVar1 != (undefined4 *)0x0) && (*(char *)(puVar2 + 1) != '\0')) {
      CloseHandle((HANDLE)*puVar1);
      *puVar1 = 0;
      *(undefined1 *)(puVar1 + 1) = 0;
      *puVar2 = 0;
      *(undefined1 *)(puVar2 + 1) = 0;
    }
    puVar2 = puVar2 + 2;
  } while ((int)puVar2 < 0x6f16d0);
  return;
}
#endif
```

## network_listen_accept_pending_connection.c

```
// network_listen_accept_pending_connection  (Ghidra: network_listen_accept_pending_connection, already named)
// address 0x4421b0, size 149 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/networking_types_notes.md "network_pending_connection (0x14)": the
// dword array `&DAT_0087bc0c` indexed by `count*5` is one network_pending_connection stride
// (0x14 == 5 dwords) *before* the documented array base (0x0087bc20), which is exactly what
// turns "pop the most recent" into `network_pending_connections[count - 1]`; the queue fields
// written (+0x00 socket, +0x0c flags bit0) match network_receive_queue.
// register convention: __cdecl, no arguments.
// UNSURE: the accept-configuration record passed to gt2Accept is a 4-dword stack
// record (result code, receive callback, two more code pointers); one of those two pointers
// (LAB_00441f30) is a raw code address in the unlifted gap between network_channel_receive_
// callback's end and network_channel_attempt_connect's start, and the other (FUN_0044ad80) is
// outside this session's address range -- both are declared only by address, never rewritten
// here, matching network_channels_open.c's precedent for gap trampolines.
// UNSURE: the trailing network_channel_get_remote_address()/network_address_to_string() calls
// are shown with no visible arguments; both callees' own files document an ESI (address-out)
// and EDI (queue) convention for the first and an EAX (address) convention for the second, so
// this rewrite introduces one local `s_network_address` scratch value to carry the pointer
// that must be threaded through both calls (ESI reused as EAX) -- the exact scratch location
// Ghidra's decompile omits, but not the effect (formatting the newly accepted peer's address).
```

```
#if 0
Original Ghidra decompilation (0x4421b0):

undefined4 * network_listen_accept_pending_connection(void)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 local_28;
  code *local_24;
  undefined1 *local_20;
  code *local_1c;

  puVar3 = (undefined4 *)0x0;
  if (0 < DAT_006f16d0) {
    local_28 = 0;
    local_24 = network_channel_receive_callback;
    local_20 = &LAB_00441f30;
    local_1c = FUN_0044ad80;
    iVar2 = thunk_FUN_0061ce80((&DAT_0087bc0c)[DAT_006f16d0 * 5],&local_28);
    if (iVar2 == 1) {
      puVar3 = network_receive_queue_new();
      if (puVar3 != (undefined4 *)0x0) {
        uVar1 = (&DAT_0087bc0c)[DAT_006f16d0 * 5];
        *puVar3 = uVar1;
        FUN_00614830(uVar1,puVar3);
        *(byte *)(puVar3 + 3) = *(byte *)(puVar3 + 3) | 1;
        network_channel_get_remote_address();
        network_address_to_string();
      }
    }
    DAT_006f16d0 = DAT_006f16d0 + -1;
  }
  return puVar3;
}
#endif
```

## network_listen_connection_request_handler.c

```
// network_listen_connection_request_handler  (Ghidra: network_listen_connection_request_handler, already named)
// address 0x442090, size 217 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md "network_pending_connection (0x14)" section:
// "network_listen_connection_request_handler (0x442090) refuses past 30 entries (0x1e <
// DAT_006f16d0) and writes five dwords at 0x0087bc20 + count*0x14 from its param_5, param_2,
// param_3, param_4 (low word) and *param_6". types/networking.h's network_error_code values 2
// and 3 match this function's two rejection paths exactly.
// register convention: none -- this is a genuine 7-parameter callback registered with the
// transport library (see network_listen_start.c), called with all parameters on the stack.
// UNSURE: the struct write of `remote_port` in the original packs it with the adjacent pad
// field into one 4-byte store whose upper 16 bits come from an uninitialized stack local
// (`local_c`); only the port itself is meaningful; the garbage that lands in the padding is not
// reproduced here.
// UNSURE: the `if (local_1c != 0)` guard on the second gt2Reject call is unreachable
// on the success path (local_1c is never set to nonzero there) but is kept exactly as
// decompiled rather than removed.
```

```
#if 0
Original Ghidra decompilation (0x442090):

void network_listen_connection_request_handler
               (undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
               undefined4 param_5,undefined4 *param_6,uint param_7)

{
  int iVar1;
  undefined4 uVar2;
  int local_1c;
  undefined1 local_18 [12];
  undefined4 local_c;

  local_1c = 0;
  if (0x1e < DAT_006f16d0) {
    local_1c = 2;
    thunk_FUN_0061cee0(param_2,&local_1c,4);
    return;
  }
  if ((param_6 != (undefined4 *)0x0) && (3 < param_7)) {
    uVar2 = *param_6;
    iVar1 = DAT_006f16d0 * 0x14;
    *(undefined4 *)(&DAT_0087bc20 + iVar1) = param_2;
    *(undefined4 *)(&DAT_0087bc24 + iVar1) = param_5;
    *(undefined4 *)(&DAT_0087bc28 + iVar1) = param_3;
    local_c = CONCAT22(local_c._2_2_,(short)param_4);
    *(undefined4 *)(&DAT_0087bc2c + iVar1) = local_c;
    *(undefined4 *)(&DAT_0087bc30 + iVar1) = uVar2;
    DAT_006f16d0 = DAT_006f16d0 + 1;
    FUN_00614820(param_1);
    FUN_006148b0(param_3,param_4,local_18);
    if (local_1c != 0) {
      thunk_FUN_0061cee0(param_2,&local_1c,4);
    }
    return;
  }
  local_1c = 3;
  thunk_FUN_0061cee0(param_2,&local_1c,4);
  return;
}
#endif
```

## network_listen_reject_pending_connection.c

```
// network_listen_reject_pending_connection  (Ghidra: FUN_00442250, still unnamed -> renamed)
// address 0x442250, size 50 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("cancels (rejects) the most recently
// queued pending incoming connection request without accepting it"); mirrors
// network_listen_accept_pending_connection.c's `network_pending_connections[count - 1]`
// derivation from the same `(&DAT_0087bc0c)[count*5]` dword-array indexing.
// register convention: __cdecl; the reject code Ghidra shows only as `&stack0x00000004` (its
// address taken, never its value read directly) is this function's own single stack
// parameter, exactly like network_listen_connection_request_handler.c's `local_1c` reject
// codes -- declared here as an ordinary by-value parameter whose address is what gets sent.
// UNSURE: the final `DAT_006f16d0 & 0xffff0000` return is always 0 for any realistic pending
// count (0..30); kept literally rather than assumed to be dead/miscompiled code.
```

```
#if 0
Original Ghidra decompilation (0x442250):

uint FUN_00442250(void)

{
  if (0 < (int)DAT_006f16d0) {
    thunk_FUN_0061cee0((&DAT_0087bc0c)[DAT_006f16d0 * 5],&stack0x00000004,4);
    DAT_006f16d0 = DAT_006f16d0 - 1;
  }
  return DAT_006f16d0 & 0xffff0000;
}
#endif
```

## network_listen_start.c

```
// network_listen_start  (Ghidra: network_listen_start, already named)
// address 0x442170, size 50 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("puts the shared network channel into
// a listening state and installs the incoming-connection-request callback"); the fields
// written (+0x04 data_ready, +0x0c flags, +0x0e last_error) match network_receive_queue; only
// caller is network_channel_new (0x4dc9b0, out of this session's range, out/phase2/
// networking/02.md) which calls this with no visible argument right after constructing the
// listening channel's endpoint queue.
// register convention: network_receive_queue * in ESI (unaff_ESI).
// UNSURE: the final `& 0xffff0000` mask on gt2Listen's return keeps only the high 16
// bits and discards the low 16, which reads backward from a typical "low half of a HRESULT/
// status word" mask; reproduced exactly rather than assumed to be a decompiler artifact.
```

```
#if 0
Original Ghidra decompilation (0x442170):

uint network_listen_start(void)

{
  uint uVar1;
  int unaff_ESI;

  *(undefined1 *)(unaff_ESI + 4) = 1;
  FUN_00614810(DAT_006f14c4);
  *(byte *)(unaff_ESI + 0xc) = *(byte *)(unaff_ESI + 0xc) | 2;
  uVar1 = thunk_FUN_0061c660(DAT_006f14c4,network_listen_connection_request_handler);
  *(undefined2 *)(unaff_ESI + 0xe) = 0;
  return uVar1 & 0xffff0000;
}
#endif
```

## network_mutex_slot_allocate.c

```
// network_mutex_slot_allocate  (Ghidra: network_mutex_slot_allocate, already named)
// address 0x440420, size 59 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/networking_types_notes.md "network_mutex_record (0x28)"; the loop
// strides `network_mutex_table` by sizeof(network_mutex_record), tests each slot's
// `in_use` byte, and the bound 0x6f12d4 against a first in-use-byte address of 0x6f0dd4
// gives exactly k_network_mutex_table_count (32) slots.
// register convention: __cdecl, no arguments.
```

```
#if 0
Original Ghidra decompilation (0x440420):

void * __cdecl network_mutex_slot_allocate(void)

{
  char *pcVar1;
  int iVar2;

  iVar2 = 0;
  pcVar1 = &DAT_006f0dd4;
  do {
    if (*pcVar1 == '\0') {
      iVar2 = iVar2 * 0x28;
      *(undefined1 *)(iVar2 + 0x6f0db4) = 0;
      *(undefined4 *)(iVar2 + 0x6f0db0) = 0;
      (&DAT_006f0dd4)[iVar2] = 1;
      return (undefined4 *)(iVar2 + 0x6f0db0);
    }
    pcVar1 = pcVar1 + 0x28;
    iVar2 = iVar2 + 1;
  } while ((int)pcVar1 < 0x6f12d4);
  return (void *)0x0;
}
#endif
```

## network_receive_queue_close_socket.c

```
// network_receive_queue_close_socket  (Ghidra: FUN_00442040, still unnamed -> renamed)
// address 0x442040, size 76 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("tears down a single channel object:
// unregisters it from the listening channel if needed, closes its handle, and resets its
// fields"); the fields it clears (socket at +0x00, flags bit0 at +0x0c) and reads
// (data_ready at +0x04) all match types/networking.h's network_receive_queue; called from
// network_receive_queue_free (0x441c80) right after the connection-oriented cleanup block, and
// from network_listen_accept_pending_connection (0x4421b0) on the rejection path.
// register convention: receive-queue pointer in ESI (unaff_ESI).
// UNSURE: the exact meaning of comparing data_ready to 1 here (rather than testing it as a
// boolean flag) is preserved literally, not reinterpreted.
```

```
#if 0
Original Ghidra decompilation (0x442040):

void FUN_00442040(void)

{
  int iVar1;
  int *unaff_ESI;

  if (((char)unaff_ESI[1] == '\x01') && (DAT_006f14c4 != 0)) {
    thunk_FUN_0061c660(DAT_006f14c4,0);
  }
  if (*unaff_ESI != 0) {
    iVar1 = FUN_006147a0(*unaff_ESI);
    if ((iVar1 == 1) || (iVar1 == 0)) {
      FUN_00614710(*unaff_ESI);
    }
  }
  *unaff_ESI = 0;
  *(byte *)(unaff_ESI + 3) = *(byte *)(unaff_ESI + 3) & 0xfe;
  return;
}
#endif
```

## network_receive_queue_free.c

```
// network_receive_queue_free  (Ghidra: network_receive_queue_free, already named)
// address 0x441c80, size 88 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("destroys a network receive-queue
// object created by network_receive_queue_new: closes its channel, frees the inner buffer and the wrapper,
// and cleans up handles"); called from network_channel_delete (0x4dcae0, out/phase2/
// networking/02.md) as `if (channel->endpoint != 0) network_receive_queue_free();`, which is
// how the EAX-as-queue-pointer convention is confirmed (channel->endpoint is loaded into the
// register the comparison just used, and the call follows immediately).
// register convention: receive-queue pointer in EAX only. The connection id (EBX/ECX for
// network_connection_stats_end) and key (DI) are NOT pass-throughs: they are the results of the
// two socket queries made just before the call (0x6175f0 -> id, 0x6147d0 -> key), so the C computes them here.


// VERIFIED against disassembly 0x441c80..0x441cd8 (2026-09-30): FIXED: connection id/key come from gamespy_array_length / gt2GetRemotePort (ebx/edi set at 0x441c95/0x441ca1), not from the caller
```

```
#if 0
Original Ghidra decompilation (0x441c80):

void network_receive_queue_free(void)

{
  int *in_EAX;

  if ((in_EAX != (int *)0x0) && (*in_EAX != 0)) {
    FUN_006175f0(*in_EAX);
    FUN_006147d0(*in_EAX);
    network_connection_stats_end();
    FUN_00614830(*in_EAX,0);
  }
  FUN_00442040();
  GlobalFree((HGLOBAL)in_EAX[4]);
  in_EAX[4] = 0;
  GlobalFree(in_EAX);
  FUN_00441bb0();
  return;
}
#endif
```

## network_receive_queue_new.c

```
// network_receive_queue_new  (Ghidra: network_receive_queue_new, already named)
// address 0x441bf0, size 136 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: out/phase4/networking_types_notes.md "network_receive_queue (0x1c)" section names
// every field this constructor writes (socket, data_ready, unknown_05, socket_key == -1,
// flags == 0, unknown_0d == 0x14, last_error == 0, incoming, unknown_14 == -1, unknown_18);
// the inner allocation is byte-for-byte types/memory.h's circular_buffer (name, 'circ'
// signature at +0x04, capacity at +0x10, data == base+0x18), named "received_data_queue" here.
// register convention: __cdecl, no arguments.
// UNSURE: none.
```

```
#if 0
Original Ghidra decompilation (0x441bf0):

void * __cdecl network_receive_queue_new(void)

{
  undefined4 *puVar1;
  undefined4 *puVar2;

  network_channels_open();
  FUN_00441bb0();
  puVar1 = GlobalAlloc(0,0x1c);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = 0;
    *(undefined1 *)(puVar1 + 1) = 0;
    *(undefined1 *)((int)puVar1 + 5) = 0;
    puVar1[2] = 0xffffffff;
    *(undefined1 *)(puVar1 + 3) = 0;
    *(undefined1 *)((int)puVar1 + 0xd) = 0x14;
    *(undefined2 *)((int)puVar1 + 0xe) = 0;
    puVar2 = GlobalAlloc(0,0x10019);
    if (puVar2 != (undefined4 *)0x0) {
      *puVar2 = 0;
      puVar2[1] = 0;
      puVar2[2] = 0;
      puVar2[3] = 0;
      puVar2[4] = 0;
      puVar2[5] = 0;
      *puVar2 = "received_data_queue";
      puVar2[1] = 0x63697263;
      puVar2[4] = 0x10001;
      puVar2[5] = puVar2 + 6;
    }
    puVar1[4] = puVar2;
    puVar1[5] = 0xffffffff;
    puVar1[6] = 0;
  }
  return puVar1;
}
#endif
```

## network_session_reject_pending_connection_callback.c

```
// network_session_reject_pending_connection_callback  (not a Ghidra function)
// address 0x4e1410, size 55 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 (retail-independence loop) from the disassembly 0x4e1410..0x4e1447. Reached only as an
// immediate: network_game_server_host_new (0x4dec40) stores it as the host session's message_callback, which the C
// did as the literal retail address 0x4e1410. The same work as network_listen_reject_pending_connection (0x442250)
// with the reject code as the second argument: while a connection request is pending (count 0x6f16d0), the newest
// one (network_pending_connections[count - 1], 0x87bc20) is rejected with the 4-byte code (gt2Reject through its
// thunk 0x614590) and the count drops by one. Returns the count as the original leaves it in EAX.
// blam-cc: cdecl (a callback: unused, reject code)
```

## network_thread_create.c

```
// network_thread_create  (Ghidra: network_thread_create, already named)
// address 0x440460, size 163 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/networking_types_notes.md "network_mutex_record (0x28) /
// network_thread_record (0x08)": the loop strides `network_thread_table` by
// sizeof(network_thread_record), and the header already documents the flag byte mapping
// ("bit1 -> -1, bit2 -> +1, else 0").
// register convention: __cdecl, all four arguments recognized by Ghidra (flags, entry point,
// thread argument, out-handle pointer).
```

```
#if 0
Original Ghidra decompilation (0x440460):

undefined4
network_thread_create
          (byte param_1,LPTHREAD_START_ROUTINE param_2,LPVOID param_3,undefined4 *param_4)

{
  undefined4 *puVar1;
  int iVar2;
  HANDLE pvVar3;
  BOOL BVar4;
  DWORD DVar5;
  DWORD local_4;

  iVar2 = 0;
  do {
    if ((&DAT_006f0cb4)[iVar2 * 8] == '\0') {
      puVar1 = (undefined4 *)(iVar2 * 8 + 0x6f0cb0);
      *puVar1 = 0;
      (&DAT_006f0cb4)[iVar2 * 8] = 1;
      pvVar3 = CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x4000,param_2,param_3,4,&local_4);
      *puVar1 = pvVar3;
      *param_4 = puVar1;
      if ((HANDLE)*puVar1 != (HANDLE)0x0) {
        iVar2 = 0;
        if ((param_1 & 2) == 0) {
          if ((param_1 & 4) != 0) {
            iVar2 = 1;
          }
        }
        else {
          iVar2 = -1;
        }
        BVar4 = SetThreadPriority((HANDLE)*puVar1,iVar2);
        if (BVar4 != 0) {
          DVar5 = ResumeThread((HANDLE)*puVar1);
          if (DVar5 != 0xffffffff) {
            return 1;
          }
        }
        CloseHandle((HANDLE)*puVar1);
      }
      return 0;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 0x20);
  return 0;
}
#endif
```
