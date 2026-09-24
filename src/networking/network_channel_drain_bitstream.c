// network_channel_drain_bitstream  (Ghidra: FUN_004e1290, unnamed)
// address 0x4e1290, size 375 bytes
// name confidence: 0.4   rewrite confidence: 0.25
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

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t network_incoming_message_scratch[0x510]; // 0x00861de0, shared scratch buffer
    // (sized from k_network_channel_stream_bits/8, see
    // network_game_process_incoming_messages.c)
extern char network_channel_incoming_read_item(network_channel *channel, uint8_t *scratch,
    uint32_t *start_bit, uint32_t *bit_count, void *out_item); // 0x4dcf10, other module
extern char network_channel_dispatch_bitstream_unit(network_server_globals *server, uint32_t bit); // 0x4e18b0, this batch

// While machine->channel has queued incoming ring-buffer data, reads one length-prefixed item
// at a time into network_incoming_message_scratch and walks it bit by bit, dispatching each
// bit through network_channel_dispatch_bitstream_unit, until an item is exhausted (down to 7
// or fewer trailing bits) or a dispatch fails.
char network_channel_drain_bitstream(network_server_globals *server, network_machine *machine)
{
    char ok;

    do {
        network_channel *channel;
        circular_buffer *incoming;
        int32_t pending;
        uint32_t start_bit;
        uint32_t bit_count;
        uint8_t item_out[24];
        uint32_t bit_cursor;
        uint32_t byte_cursor;
        uint32_t end_bit;
        uint32_t cur_bit;
        uint32_t cur_byte;
        uint32_t cur_end;
        uint32_t cur_start;
        uint32_t remaining;

        channel = machine->channel;
        if (channel == 0) {
            return 1;
        }
        incoming = channel->incoming;
        pending = incoming->write_cursor - incoming->read_cursor;
        if (pending < 0) {
            pending = pending + incoming->capacity;
        }
        if (pending == 0) {
            return 1;
        }

        start_bit = 0;
        bit_count = 0;
        ok = network_channel_incoming_read_item(channel, network_incoming_message_scratch,
                                                 &start_bit, &bit_count, item_out);
        if (ok == 0) {
            return 0;
        }

        bit_cursor = start_bit & 7;
        byte_cursor = start_bit >> 3;
        end_bit = bit_count - 1 + start_bit;
        remaining = bit_count;
        cur_bit = bit_cursor;
        cur_byte = byte_cursor;
        cur_end = end_bit;
        cur_start = start_bit;

        while (ok == 1 && (int32_t)((cur_start - cur_byte * 8 - cur_bit) + remaining) > 7) {
            char have_bit;
            uint32_t bit_value;
            uint32_t abs_pos;

            have_bit = 0;
            bit_value = 0;
            abs_pos = cur_byte * 8 + cur_bit;
            if (cur_start <= abs_pos && abs_pos <= cur_end) {
                bit_value = (uint32_t)((network_incoming_message_scratch[cur_byte] >> (cur_bit & 0x1f)) & 1);
                abs_pos = abs_pos + 1;
                if ((cur_start <= abs_pos && abs_pos <= cur_end) || abs_pos == cur_end + 1) {
                    cur_bit = abs_pos & 7;
                    cur_byte = abs_pos >> 3;
                    bit_cursor = cur_bit;
                    byte_cursor = cur_byte;
                }
                have_bit = 1;
            }
            ok = 0;
            if (have_bit) {
                ok = network_channel_dispatch_bitstream_unit((network_server_globals *)server, bit_value);
                cur_bit = bit_cursor;
                cur_byte = byte_cursor;
                cur_end = end_bit;
                cur_start = start_bit;
            }
        }
    } while (ok != 0);
    return 0;
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
