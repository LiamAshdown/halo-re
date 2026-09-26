// network_game_server_send_message_to_all_machines  (Ghidra: already named)
// address 0x4e4e30, size 192 bytes
// name confidence: 0.7   rewrite confidence: 0.35
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

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern network_client_globals *network_client; // 0x0071c2d8

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern uint8_t network_channel_stream_flush(network_channel *channel, int32_t mode); // this module (earlier batch), 0x4ddb60
extern int32_t bit_stream_write_bits_chunked(int32_t total_bit_count, uint32_t value,
    bit_stream *stream); // 0x4cf8f0, memory module (src/memory/bit_stream_write_bits_chunked.c);
    // UNSURE: only the bit count is visible at the call sites below, the value and the stream
    // operand are in registers Ghidra dropped, so both are passed as 0 here.

// Copies first_string and second_string into the two-field message body, encodes it as message
// type 0x36 and, budget permitting, appends it to network_client's outgoing channel stream.
void network_game_server_send_message_to_all_machines(char *first_string, int32_t unsure_offset,
    char *second_string) // blam-cc: EAX -> first_string, EDX -> unsure_offset (unused), EDI -> second_string
{
    char first_buf[16]; // UNSURE: exact size; Ghidra's own stack frame for this batch of copies
    char second_buf[16]; // is not otherwise recovered
    void *fields[3];
    int32_t encoded_bits;

    strcpy(first_buf, first_string);
    strcpy(second_buf, second_string);
    fields[0] = second_buf;
    fields[1] = 0;
    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x36, 0, fields, 0, 1, 0);
    if (0 < encoded_bits) {
        network_channel *channel = network_client->channel;
        int32_t free_bits = channel->outgoing.stream.last_bit -
            channel->outgoing.stream.byte_cursor * 8 - channel->outgoing.stream.bit_cursor + 1;
        if ((channel->flags & 1) == 0 &&
            (encoded_bits + 1 <= free_bits || network_channel_stream_flush(channel, 1) != 0)) {
            channel->send_budget = channel->send_budget + encoded_bits + 1;
            bit_stream_write_bits_chunked(1, 0, 0); // UNSURE: value/stream elided
            channel->outgoing.empty = 0;
            bit_stream_write_bits_chunked(encoded_bits, 0, 0); // UNSURE: value/stream elided
            channel->outgoing.empty = 0;
        }
    }
}

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
