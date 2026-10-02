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

#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t network_incoming_message_scratch[0x510]; // 0x00861de0, UNSURE size
extern int32_t network_channel_incoming_read_item(network_channel *channel, uint8_t *destination,
    int32_t *out_bit_offset, int32_t *out_remaining_bits, s_network_address *out_address,
    int32_t max_item_bits); // 0x4dcf10, stack x5, EAX max bits
extern char network_incoming_item_dispatch(network_client_globals *client, uint32_t item_flag, bit_stream *stream,
    const uint32_t *sender); // 0x4db630, stack, stack, ECX, ESI

typedef struct network_item_stream {
    bit_stream stream;             // 0x00
    uint32_t bit_count;            // 0x18
} network_item_stream;

int32_t network_game_process_incoming_messages(network_client_globals *client)
{
    char result = 1;

    for (;;) {
        network_channel *channel = client->channel;
        circular_buffer *incoming = channel->incoming;
        int32_t available;
        int32_t bit_offset = 0;
        int32_t bit_count = 0;
        uint32_t sender[6];
        network_item_stream s;

        if (incoming == 0) {
            return result;
        }
        available = incoming->write_cursor - incoming->read_cursor;
        if (available < 0) {
            available += incoming->capacity;
        }
        if (available == 0) {
            return result;
        }
        result = (char)network_channel_incoming_read_item(channel, network_incoming_message_scratch, &bit_offset,
                                                          &bit_count, (s_network_address *)sender, 0x80000);
        if (result == 0) {
            continue;
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
                result = network_incoming_item_dispatch(client, item_flag, &s.stream, sender);
            } while (result == 1);
        }
        result = result != 0;
    }
}

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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
