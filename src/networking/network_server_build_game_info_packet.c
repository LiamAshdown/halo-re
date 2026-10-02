// network_server_build_game_info_packet  (Ghidra: FUN_004e0950, unnamed)
// address 0x4e0950, size 345 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Builds and queues a small 'game info' style
// packet (short name plus a game-data snapshot) for the channel referenced by param_2, encoded
// as message type 4." param_2+0x0/+0xc match network_machine::channel/machine_id;
// param_1+0x88 (server + 0x88 == session + 0x80) copies exactly 0x84 bytes -- session's
// unknown_080, server_name[64] and unknown_0c4[0x40] back to back -- into a scratch snapshot.
// The free-space/flush sequence on the result matches network_channel::outgoing
// (bit_stream last_bit/byte_cursor/bit_cursor at +0x24/+0x1c/+0x20) and
// network_channel_stream_flush's own documented (stream, channel, mode) call shape.
// register convention: stack = server (network_server_globals *), machine (network_machine *).
// blam-cc: stack -> server, machine
// UNSURE: param_2's +0x52..+0x59 region is declared in types/networking.h as two separate
// "unaligned int32" fields (unknown_52, unknown_56) on network_machine; this function's
// 7-character-plus-NUL strncpy into exactly that 8-byte span strongly suggests it is really a
// `char short_name[8]`, but the header is not edited here -- accessed through a raw offset
// instead, with this note standing in for a TYPES-GAP.
// UNSURE: Ghidra's individual locals (local_6a6, local_6a4, local_6a0[7], local_699, local_698,
// local_694, local_690[419]) are contiguous on the stack in exactly that order (each one's
// ebp-relative offset abuts the next), so `local_6a0` -- the sole pointer passed to
// data_packet_group_encode_packet -- is really the address of one combined record: 7-byte
// short name, NUL, a flag byte, 3 bytes of padding, a machine-id dword, then the 0x84-byte
// snapshot, with roughly 1.5KB of additional scratch after it (up to the 0x600-byte size cap
// passed alongside). Modelled here as one struct-shaped local instead of Ghidra's separate
// variables.
// UNSURE: FUN_00575fa0 (source of the machine's short name copied at param_2+0x52 -- wait, see
// below), DAT_006894a2, FUN_004cf8f0 and the exact meaning of the packed word returned by
// network_message_block_build (`(*result >> 4) * 8` as a bit length) are not independently confirmed.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from encoded_buffer; the C passed placeholders or dropped the arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char *autopatch_temp_name_generate(void); // other module (UNSURE): source string for the machine's short name
extern uint8_t network_game_info_packet_flag; // 0x006894a2 (UNSURE name)
extern char data_packet_group_encode_packet(void *header, uint32_t *size_in_out, int32_t group, int32_t message_type); // 0x4d0ae0, this module family
extern uint16_t *network_message_block_build(uint32_t size); // 0x440350, this module (UNSURE: packs the
    // just-encoded message into a newly allocated buffer; see this module's very first
    // function for the closest available context)
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits

// Stamps `machine`'s short name and a snapshot of the server's name/game-data block, encodes
// them as a type-4 "game info" message, and queues the encoded bits onto machine->channel's
// outgoing stream, growing the reliable queue via network_channel_stream_flush if there is not
// enough room.
typedef struct network_game_info_record { // UNSURE: reconstructed stack layout, see file header
    char short_name[7];
    uint8_t nul;
    uint8_t flag;
    uint8_t pad[3];
    int32_t machine_id;
    uint8_t snapshot[0x84];
    uint8_t scratch[0x600]; // room for encode_packet's output, sized to the 0x600 cap below
} network_game_info_record;

char network_server_build_game_info_packet(network_server_globals *server, network_machine *machine)
{
    network_game_info_record record;
    uint32_t size;
    char *source_name;
    char encoded;

    source_name = autopatch_temp_name_generate();
    strncpy((char *)machine + 0x52, source_name, 7); // UNSURE: network_machine+0x52 short_name[8]
    *((char *)machine + 0x59) = 0;

    strncpy(record.short_name, (char *)machine + 0x52, 7);
    record.nul = 0;
    memcpy(record.snapshot, (uint8_t *)server + 0x88, sizeof(record.snapshot));
    record.flag = network_game_info_packet_flag;
    record.machine_id = (int32_t)machine->machine_id;

    size = 0x600;
    encoded = data_packet_group_encode_packet(&record, &size, 4, 1);
    if (encoded != 0) {
        uint16_t *encoded_buffer;

        encoded_buffer = network_message_block_build(size);
        if (encoded_buffer != 0) {
            network_channel *channel;

            channel = machine->channel;
            if (channel != 0) {
                int32_t bit_len;
                char result;

                bit_len = (int32_t)(*encoded_buffer >> 4) * 8;
                result = 1;
                if ((channel->flags & 0x01) == 0) {
                    int32_t free_bits;

                    free_bits = (int32_t)(channel->outgoing.stream.last_bit -
                                          channel->outgoing.stream.byte_cursor * 8) -
                                (int32_t)channel->outgoing.stream.bit_cursor + 1;
                    if (free_bits < bit_len + 1) {
                        result = network_channel_stream_flush(&channel->outgoing, channel, 1);
                        if (result == 0) {
                            return 0;
                        }
                    }
                    channel->send_budget = channel->send_budget + bit_len + 1;
                    { uint32_t item_flag = 0; bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
                    channel->outgoing.empty = 0;
                    bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(encoded_buffer), bit_len);
                    channel->outgoing.empty = 0;
                }
                return result;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4e0950):

char FUN_004e0950(int param_1,int *param_2)

{
  int iVar1;
  char cVar2;
  char *_Source;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  size_t _Count;
  char local_6a6;
  ushort *local_6a4;
  char local_6a0 [7];
  undefined1 local_699;
  undefined1 local_698;
  int local_694;
  undefined4 local_690 [419];

  _Count = 7;
  _Source = (char *)FUN_00575fa0();
  _strncpy((char *)((int)param_2 + 0x52),_Source,_Count);
  *(undefined1 *)((int)param_2 + 0x59) = 0;
  iVar1 = param_2[3];
  _strncpy(local_6a0,(char *)((int)param_2 + 0x52),7);
  local_699 = 0;
  puVar4 = (undefined4 *)(param_1 + 0x88);
  puVar5 = local_690;
  for (iVar3 = 0x21; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  local_698 = DAT_006894a2;
  local_6a4 = (ushort *)0x600;
  local_694 = (int)(short)iVar1;
  cVar2 = data_packet_group_encode_packet(local_6a0,&local_6a4,4,1);
  if ((cVar2 != '\0') && (local_6a4 = (ushort *)FUN_00440350(local_6a4), local_6a4 != (ushort *)0x0)
     ) {
    iVar1 = *param_2;
    iVar3 = (uint)(*local_6a4 >> 4) * 8;
    if (iVar1 != 0) {
      local_6a6 = '\x01';
      if ((*(byte *)(iVar1 + 0xa8c) & 1) == 0) {
        if ((((*(int *)(iVar1 + 0x24) + *(int *)(iVar1 + 0x1c) * -8) - *(int *)(iVar1 + 0x20)) + 1 <
             iVar3 + 1) && (local_6a6 = FUN_004ddb60(iVar1,1), local_6a6 == '\0')) {
          return '\0';
        }
        *(int *)(iVar1 + 0xa80) = *(int *)(iVar1 + 0xa80) + iVar3 + 1;
        FUN_004cf8f0(1);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
        FUN_004cf8f0(iVar3);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
      }
      return local_6a6;
    }
  }
  return '\0';
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
