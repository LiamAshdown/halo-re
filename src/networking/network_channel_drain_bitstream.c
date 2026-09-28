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

#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t network_incoming_message_scratch[0x510]; // 0x00861de0, UNSURE size
extern int32_t network_channel_incoming_read_item(network_channel *channel, uint8_t *destination,
    int32_t *out_bit_offset, int32_t *out_remaining_bits, s_network_address *out_address,
    int32_t max_item_bits); // 0x4dcf10, stack x5, EAX max bits
extern char network_channel_dispatch_bitstream_unit(network_server_globals *server, uint32_t unit, bit_stream *stream,
    network_machine *machine); // 0x4e18b0, stack, stack, ECX, ESI

typedef struct network_item_stream {
    bit_stream stream;             // 0x00
    uint32_t bit_count;            // 0x18
} network_item_stream;

char network_channel_drain_bitstream(network_server_globals *server, network_machine *machine)
{
    char result = 1;

    for (;;) {
        network_channel *channel = *(network_channel **)machine;
        circular_buffer *incoming = channel->incoming;
        int32_t available;
        int32_t bit_offset = 0;
        int32_t bit_count = 0;
        uint32_t sender[6];
        network_item_stream s;

        if (incoming == 0) {
            return 1;
        }
        available = incoming->write_cursor - incoming->read_cursor;
        if (available < 0) {
            available += incoming->capacity;
        }
        if (available == 0) {
            return 1;
        }
        result = (char)network_channel_incoming_read_item(channel, network_incoming_message_scratch, &bit_offset,
                                                          &bit_count, (s_network_address *)sender, 0x80000);
        if (result == 0) {
            return 0;
        }
        s.stream.unknown_00 = 1;
        s.stream.data = network_incoming_message_scratch;
        s.stream.first_bit = (uint32_t)bit_offset;
        s.stream.byte_cursor = (uint32_t)bit_offset >> 3;
        s.stream.bit_cursor = (uint32_t)bit_offset & 7;
        s.stream.last_bit = (uint32_t)(bit_count + bit_offset - 1);
        s.bit_count = (uint32_t)bit_count;
        if (result == 1) {
            do {
                uint32_t position = s.stream.byte_cursor * 8 + s.stream.bit_cursor;
                uint32_t next;
                uint8_t item_flag;

                if (s.stream.first_bit - s.stream.byte_cursor * 8 - s.stream.bit_cursor + (uint32_t)bit_count < 8) {
                    break;
                }
                if (position < s.stream.first_bit || position > s.stream.last_bit) {
                    result = 0;
                    break;
                }
                item_flag = (uint8_t)((s.stream.data[s.stream.byte_cursor] >> s.stream.bit_cursor) & 1);
                next = position + 1;
                if ((next >= s.stream.first_bit && next <= s.stream.last_bit) || next == s.stream.last_bit + 1) {
                    s.stream.byte_cursor = next >> 3;
                    s.stream.bit_cursor = next & 7;
                }
                result = network_channel_dispatch_bitstream_unit(server, item_flag, &s.stream, machine);
            } while (result == 1);
        }
        if (result == 0) {
            return 0;
        }
    }
}

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
