// rcon_send_request  (Ghidra: rcon_send_request, already named)
// address 0x4e4dc0, size 304 bytes (0x4e4dc0..0x4e4ef8; was recorded as 112, see the FIXED note)
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/networking_functions.md; the "ERROR: Maximum rcon %s length" strings;
// this batch's rcon.c disassembly (objdump -d -M intel) shows the call site setting EAX to the
// rebuilt command buffer and ECX to the original password argument right before the call.
// register convention: EAX -> command, ECX -> password.
//   // blam-cc: EAX -> command, ECX -> password
// UNSURE: this function reaches into network_client's channel (client+0xadc) and its outgoing
// stream (channel+0x010, see types/networking.h network_channel/network_channel_stream) using
// raw offsets, matching the shape network_game_server_send_message_to_all_machines.c (this
// batch) also uses; message_delta_encode_message and bit_stream_write_bits_chunked's exact
// signatures are not resolved by this batch (the message-delta protocol is documented in
// types/networking.h as only partly resolved), so both are declared with the minimal shape this
// call site needs and their extra arguments are passed through as opaque ints/pointers.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_memory.h"
#include <string.h>

extern network_client_globals *network_client; // 0x0071c2d8

extern void *global_white_argb; // 0x006851fc, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)
extern int32_t message_delta_encode_message(int32_t buffer, int32_t bit_budget, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX, EDX
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, ESI stream (channel +0x10), stack channel, mode


// FIXED 2026-09-28 (send-path audit, from the disassembly 0x4e4dc0..0x4e4ef8 -- the function is 0x130 bytes; the
// "function" at 0x4e4e30 is the loop label of its first string copy): the password and the command are copied into
// ONE record {password[9], command[0x41]} which is encoded (message type 0x36, one item) into the network scratch
// 0x871de0, then written to the client channel's outgoing stream as a game-action item (flag 1). The C encoded two
// separate arrays through a malformed call and wrote placeholders.
typedef struct rcon_request_record {
    char password[9];              // 0x00
    char command[0x41];            // 0x09
} rcon_request_record;

void rcon_send_request(char *command, char *password) // blam-cc: EAX -> command, ECX -> password
{
    rcon_request_record record;
    void *items[2];
    int32_t encoded_bits;

    if (strlen(password) > 8) {
        chimera__console_out((ColorARGB *)global_white_argb, "ERROR: Maximum rcon password length is %d characters", 8);
        return;
    }
    if (strlen(command) > 0x40) {
        chimera__console_out((ColorARGB *)global_white_argb, "ERROR: Maximum rcon command length is %d characters", 0x40);
        return;
    }
    strcpy(record.password, password);
    strcpy(record.command, command);
    items[0] = &record;
    items[1] = 0;
    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x36, 0, items, 0, 1, 0);
    if (encoded_bits > 0) {
        network_channel *channel = network_client->channel;
        bit_stream *stream = (bit_stream *)((uint8_t *)channel + 0x10);
        int32_t free_bits = channel->outgoing.stream.last_bit -
            channel->outgoing.stream.byte_cursor * 8 - channel->outgoing.stream.bit_cursor + 1;

        if ((channel->flags & 1) == 0 &&
            (encoded_bits + 1 <= free_bits || network_channel_stream_flush((network_channel_stream *)((uint8_t *)channel + 0x10), (network_channel *)channel, 1) != 0)) {
            uint32_t item_flag = 1;

            channel->send_budget = channel->send_budget + encoded_bits + 1;
            bit_stream_write_bits_chunked(stream, &item_flag, 1);
            channel->outgoing.empty = 0;
            bit_stream_write_bits_chunked(stream, (const uint32_t *)network_message_scratch, encoded_bits);
            channel->outgoing.empty = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4e4dc0), from tools/pack.py 0x4e4dc0:

void rcon_send_request(void)

{
  int iVar1;
  char cVar2;
  char *in_EAX;
  char *pcVar3;
  int iVar4;
  char *in_ECX;
  char *pcStack_54;
  undefined4 uStack_50;
  char local_4c [9];
  char acStack_43 [67];

  pcVar3 = in_ECX;
  do {
    cVar2 = *pcVar3;
    pcVar3 = pcVar3 + 1;
  } while (cVar2 != '\0');
  if (8 < (uint)((int)pcVar3 - (int)(in_ECX + 1))) {
    chimera__console_out("ERROR: Maximum rcon password length is %d characters",8);
    return;
  }
  pcVar3 = in_EAX;
  do {
    cVar2 = *pcVar3;
    pcVar3 = pcVar3 + 1;
  } while (cVar2 != '\0');
  if ((uint)((int)pcVar3 - (int)(in_EAX + 1)) < 0x41) {
    iVar4 = -(int)in_ECX;
    do {
      cVar2 = *in_ECX;
      in_ECX[(int)(local_4c + iVar4)] = cVar2;
      in_ECX = in_ECX + 1;
    } while (cVar2 != '\0');
    iVar4 = -(int)in_EAX;
    do {
      cVar2 = *in_EAX;
      in_EAX[(int)(acStack_43 + iVar4)] = cVar2;
      in_EAX = in_EAX + 1;
    } while (cVar2 != '\0');
    pcStack_54 = local_4c;
    uStack_50 = 0;
    iVar4 = message_delta_encode_message(0,0x36,0,&pcStack_54,0,1,'\0');
    if (0 < iVar4) {
      iVar1 = *(int *)(DAT_0071c2d8 + 0xadc);
      if (((*(byte *)(iVar1 + 0xa8c) & 1) == 0) &&
         ((iVar4 + 1 <=
           ((*(int *)(iVar1 + 0x24) + *(int *)(iVar1 + 0x1c) * -8) - *(int *)(iVar1 + 0x20)) + 1 ||
          (cVar2 = FUN_004ddb60(iVar1,1), cVar2 != '\0')))) {
        *(int *)(iVar1 + 0xa80) = *(int *)(iVar1 + 0xa80) + iVar4 + 1;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar4);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
      }
    }
    return;
  }
  chimera__console_out("ERROR: Maximum rcon command length is %d characters",0x40);
  return;
}
#endif
