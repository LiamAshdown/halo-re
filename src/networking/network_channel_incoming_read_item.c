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
// UNSURE (significant): `in_EAX`, the hidden bit-count bound the decoded length is checked
// against (via a ceil-divide-by-8 idiom), could not be identified; modeled as an explicit
// parameter (max_item_bits) rather than guessed at a specific global or field.
// UNSURE: the exact stack layout feeding bit_stream_read_bits_chunked's hidden `buffer` and
// `stream` arguments is reconstructed from local variable adjacency (a 6-dword bit_stream
// immediately followed by a 7th dword matching a 16-bit chunk read), not observed directly.
// register/parameter convention: max_item_bits in EAX (elided). blam-cc: EAX -> max_item_bits,
// stack -> channel, destination, out_bit_offset, out_remaining_length_bits, out_address

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_bit_chunk_size; // 0x0071c2cc
extern uint32_t circular_buffer_read(uint8_t *destination, uint32_t byte_count, char consume,
    circular_buffer *stream); // 0x4d0240, memory module
extern int32_t bit_stream_read_bits_chunked(int32_t total_bit_count, uint32_t *buffer, bit_stream *stream); // 0x4cf950, memory module
extern int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue); // 0x441ce0, this module

// blam-cc: EAX -> max_item_bits
int32_t network_channel_incoming_read_item(network_channel *channel, uint8_t *destination,
    int32_t *out_bit_offset, int32_t *out_remaining_bits, s_network_address *out_address,
    int32_t max_item_bits)
{
    circular_buffer *incoming;
    int32_t available;
    uint8_t peeked[4];
    int32_t item_length;
    bit_stream length_stream;
    int32_t chunk_size;
    int32_t consumed_bits;
    int32_t max_item_bytes;

    incoming = channel->incoming;
    if (incoming == 0) {
        return 0;
    }
    available = incoming->write_cursor - incoming->read_cursor;
    if (available < 0) {
        available = available + incoming->capacity;
    }
    if (available <= 0) {
        return 0;
    }
    item_length = 0;
    if (circular_buffer_read(peeked, 2, 0, incoming) == 0) {
        return 0;
    }
    chunk_size = network_bit_chunk_size;
    length_stream.unknown_00 = 1;
    length_stream.data = peeked;
    length_stream.first_bit = 0;
    length_stream.byte_cursor = 0;
    length_stream.bit_cursor = 0;
    length_stream.last_bit = 0xf;
    consumed_bits = bit_stream_read_bits_chunked(0x10, (uint32_t *)&item_length, &length_stream);
    if (consumed_bits != chunk_size) {
        return 0;
    }
    if (item_length > 1) {
        // Ghidra's ceil(max_item_bits/8) is a signed-division idiom; simplified to the
        // equivalent (bits>>3)+((bits&7)!=0) already used elsewhere in this batch, since
        // max_item_bits is not expected to be negative in practice.
        max_item_bytes = (max_item_bits >> 3) + ((max_item_bits & 7) != 0);
        if (item_length <= max_item_bytes) {
            int32_t available2 = incoming->write_cursor - incoming->read_cursor;
            if (available2 < 0) {
                available2 = available2 + incoming->capacity;
            }
            if (item_length <= available2) {
                circular_buffer_read(destination, item_length, 1, incoming);
                if (out_address != 0) {
                    network_channel_get_remote_address(out_address, channel->endpoint);
                    if (channel->endpoint->last_error != 0) {
                        out_address->ipv4 = 0;
                        out_address->ipv6_1 = 0;
                        out_address->ipv6_2 = 0;
                        out_address->ipv6_3 = 0;
                        out_address->size = k_network_address_size_ipv4;
                        out_address->port = 0;
                    }
                }
                *out_bit_offset = chunk_size;
                *out_remaining_bits = item_length * 8 - chunk_size;
                return 1;
            }
        }
    }
    incoming->write_cursor = 0;
    incoming->read_cursor = 0;
    return 0;
}

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
