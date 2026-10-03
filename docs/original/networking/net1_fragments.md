# Original notes: networking `net1_fragments`

Author notes and decompile blocks moved verbatim from the C files converted into the net1_fragments sources.

## network_game_server_send_message_to_all_machines.c

```
// network_game_server_send_message_to_all_machines  (Ghidra: already named)
// address 0x4e4e30, size 192 bytes
// name confidence: 0.7   rewrite confidence: n/a (FRAGMENT)
// evidence: out/phase4/networking_functions.md ("Server-side helper that queues a formatted
// text message for broadcast to all connected client machines"); shares the exact
// free-space/send-budget/empty-flag sequence with rcon_send_request.c (this batch), which is
// what pins network_client->channel's outgoing stream fields here. This function has zero
// callers found by static analysis (out/functions.json: "callers":0), so it may be reached only
// through a function pointer or hook this batch's evidence does not cover.
// register convention: Ghidra recovers only EAX and unaff_EDI as string sources and unaff_EDX as
// a value used solely as `in_EAX[in_EDX] = ...`, the same "copy to a nearby stack buffer"
// pointer-difference idiom rcon_send_request.c's disassembly resolved to a plain strcpy with no
// real third parameter; lacking a caller to disassemble here, EDX is kept as an UNSURE opaque
// parameter this rewrite does not otherwise use.
//   // blam-cc: EAX -> first_string, EDX -> unsure_offset (unused), EDI -> second_string
// UNSURE: which of first_string/second_string is the sender name vs. the message text; both are
// simply copied verbatim into the two message fields. UNSURE: EDX's role (see above).

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (1: a game action) from a local, then
// the encoded bits from network_message_scratch 0x871de0; the C passed placeholders or dropped the arguments.
// FRAGMENT (2026-09-28, from the disassembly): 0x4e4e30 is not a function -- it is the loop label of the first string
// copy inside rcon_send_request (0x4e4dc0..0x4e4ef8; `jne 0x4e4e30` at 0x4e4e38), which is why nothing calls it.
// rcon_send_request.c covers the whole range; nothing is translated here, so the standalone link gives it no code
// entry.
```

```
#if 0
Original Ghidra decompilation (0x4e4e30), from tools/pack.py 0x4e4e30:

void network_game_server_send_message_to_all_machines(void)

{
  int iVar1;
  char cVar2;
  char *in_EAX;
  int iVar3;
  int in_EDX;
  char *unaff_EDI;
  undefined1 uStack0000000b;
  undefined1 *puStack0000000c;
  undefined4 uStack00000010;

  do {
    cVar2 = *in_EAX;
    in_EAX[in_EDX] = cVar2;
    in_EAX = in_EAX + 1;
  } while (cVar2 != '\0');
  iVar3 = -(int)unaff_EDI;
  do {
    cVar2 = *unaff_EDI;
    unaff_EDI[(int)(&stack0x0000001d + iVar3)] = cVar2;
    unaff_EDI = unaff_EDI + 1;
  } while (cVar2 != '\0');
  puStack0000000c = &stack0x00000014;
  uStack00000010 = 0;
  iVar3 = message_delta_encode_message(0,0x36,0,&stack0x0000000c,0,1,'\0');
  if (0 < iVar3) {
    iVar1 = *(int *)(DAT_0071c2d8 + 0xadc);
    uStack0000000b = 1;
    if (((*(byte *)(iVar1 + 0xa8c) & 1) == 0) &&
       ((iVar3 + 1 <=
         ((*(int *)(iVar1 + 0x24) + *(int *)(iVar1 + 0x1c) * -8) - *(int *)(iVar1 + 0x20)) + 1 ||
        (cVar2 = FUN_004ddb60(iVar1,1), cVar2 != '\0')))) {
      *(int *)(iVar1 + 0xa80) = *(int *)(iVar1 + 0xa80) + iVar3 + 1;
      bit_stream_write_bits_chunked(1);
      *(undefined1 *)(iVar1 + 0x2c) = 0;
      bit_stream_write_bits_chunked(iVar3);
      *(undefined1 *)(iVar1 + 0x2c) = 0;
    }
  }
  return;
}
#endif
```
