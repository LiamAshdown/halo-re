// network_game_process_incoming_messages  (Ghidra: network_game_process_incoming_messages,
// already named)
// address 0x4db180, size 388 bytes
// name confidence: 0.6   rewrite confidence: 0.4
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

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t network_incoming_message_scratch[0x510]; // 0x00861de0, UNSURE size; see file header
extern char network_channel_incoming_read_item(network_channel *channel, uint8_t *scratch, uint32_t *out_bit_offset,
    int32_t *out_byte_count, uint8_t *out_item); // 0x4dcf10, this batch (not yet rewritten at
                                                  // time of writing)
extern char network_incoming_item_dispatch(network_client_globals *client, uint32_t item_flag); // 0x4db630, this batch

// blam-cc: stack -> client
int32_t network_game_process_incoming_messages(network_client_globals *client)
{
    circular_buffer *incoming;
    int32_t available;
    uint32_t bit_offset;
    int32_t byte_count;
    uint8_t item[24];
    char ok;
    int32_t result;

    result = 1;
    while (1) {
        incoming = client->channel->incoming;
        if (incoming == 0) {
            return result;
        }
        available = incoming->write_cursor - incoming->read_cursor;
        if (available < 0) {
            available = available + incoming->capacity;
        }
        if (available == 0) {
            break;
        }

        bit_offset = 0;
        byte_count = 0;
        ok = network_channel_incoming_read_item(client->channel, network_incoming_message_scratch, &bit_offset,
                           &byte_count, item);
        result = 0;
        if (ok != 0) {
            uint32_t bit_index_low, byte_index, end_bit, cursor;
            char dispatch_ok;
            int32_t dispatched;

            bit_index_low = bit_offset & 7;
            byte_index = bit_offset >> 3;
            end_bit = byte_count - 1 + bit_offset;
            cursor = bit_offset;
            dispatch_ok = ok;

            while (dispatch_ok == 1 && 7 < ((cursor - byte_index * 8) - bit_index_low) + byte_count) {
                uint32_t abs_bit;
                uint32_t item_flag;

                result = 0;
                abs_bit = byte_index * 8 + bit_index_low;
                item_flag = 0;
                dispatched = 0;
                if (cursor <= abs_bit && abs_bit <= end_bit) {
                    item_flag = (uint32_t)(network_incoming_message_scratch[byte_index] &
                                            (1 << (bit_index_low & 0x1f))) >>
                                (bit_index_low & 0x1f) & 0xff;
                    abs_bit = abs_bit + 1;
                    if ((cursor <= abs_bit && abs_bit <= end_bit) || abs_bit == end_bit + 1) {
                        bit_index_low = abs_bit & 7;
                        byte_index = abs_bit >> 3;
                    }
                    dispatched = 1;
                }
                dispatch_ok = 0;
                if (dispatched) {
                    dispatch_ok = network_incoming_item_dispatch(client, item_flag);
                    cursor = bit_offset;
                }
            }
            result = dispatch_ok != 0;
        }
    }
    return result;
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
