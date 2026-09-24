// rcon_send_request  (Ghidra: rcon_send_request, already named)
// address 0x4e4dc0, size 112 bytes
// name confidence: 0.6   rewrite confidence: 0.45
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
#include <string.h>

extern network_client_globals *network_client; // 0x0071c2d8

extern void chimera__console_out(const char *format, ...); // 0x496b50
extern int32_t message_delta_encode_message(int32_t a, int32_t message_type, int32_t b,
    void *fields, int32_t c, int32_t d, char e); // this module (later batch), 0x4ec940, UNSURE shape
extern uint8_t network_channel_stream_flush(network_channel *channel, int32_t mode); // this module (earlier batch), 0x4ddb60
extern int32_t bit_stream_write_bits_chunked(int32_t total_bit_count, uint32_t value,
    bit_stream *stream); // 0x4cf8f0, memory module (src/memory/bit_stream_write_bits_chunked.c);
    // UNSURE: only the bit count is visible at the call sites below, the value and the stream
    // operand are in registers Ghidra dropped, so both are passed as 0 here.

// Builds and transmits an rcon-request message containing `password` and `command` to the
// server, after validating both fit their maximum lengths (8 and 64 characters).
void rcon_send_request(char *command, char *password) // blam-cc: EAX -> command, ECX -> password
{
    char password_buf[9];
    char command_buf[68];
    void *fields[2];
    int32_t encoded_bits;

    if (8 < (int32_t)strlen(password)) {
        chimera__console_out("ERROR: Maximum rcon password length is %d characters", 8);
        return;
    }
    if ((int32_t)strlen(command) < 0x41) {
        strcpy(password_buf, password);
        strcpy(command_buf, command);
        fields[0] = password_buf;
        fields[1] = 0;
        encoded_bits = message_delta_encode_message(0, 0x36, 0, fields, 0, 1, 0);
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
        return;
    }
    chimera__console_out("ERROR: Maximum rcon command length is %d characters", 0x40);
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
