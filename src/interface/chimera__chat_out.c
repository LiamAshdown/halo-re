// chimera__chat_out  (Ghidra: chimera__chat_out, already named)
// address 0x4aab00, size 197 bytes
// name confidence: 0.55 (existing Ghidra name)   rewrite confidence: 0.15
// evidence: matches the given name and cc (__cdecl); reuses message_delta_encode_message and
// network_message_scratch (0x00871de0), both established in src/effects/
// player_effect_send_network_update.c; bit_stream_write_bits_chunked established in
// src/memory/.
// UNSURE: this is one of the least-confident rewrites in this pass. Ghidra's own output carries
// a "globals starting with '_' overlap smaller symbols at the same address" warning, and the
// pseudocode writes a "_channel" global at three different sizes that do not correspond to any
// real, consistently-typed symbol; this is almost certainly Ghidra mis-labeling reuses of the
// `channel` parameter's own stack slot, not a second variable, so it is transcribed here as
// plain locals instead. The network-session struct fields reached through
// `*(int*)(DAT_0071c2d8+0xadc)` (offsets 0x1c, 0x20, 0x24, 0x2c, 0xa80, 0xa8c) are kept as raw
// offsets; no networking struct was brought into scope for this rewrite, and
// message_delta_encode_message's own "fields" argument here is a bare `&channel` (a single
// byte), unlike its struct-pointer usage in the established caller.
// register convention: __cdecl, channel as the recognized parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern network_client_globals *network_client; // 0x0071c2d8, UNSURE: pointer to a session struct, offset 0xadc read from it

extern int32_t message_delta_encode_message(uint32_t unknown_0, uint32_t message_type, uint32_t unknown_2,
                                             void **fields, uint32_t unknown_4, uint32_t unknown_5,
                                             uint8_t unknown_6); // 0x4ec940
extern uint8_t network_channel_stream_flush(uint8_t *session, int32_t unknown); // 0x4ddb60, UNSURE signature
extern void bit_stream_write_bits_chunked(uint32_t value_or_count); // 0x4cf8f0, UNSURE: elided second argument

// Encodes a chat text message (type 0xf) for the given channel and, if the session's outgoing
// buffer has room (or can be flushed to make room), queues its length and payload bits for
// network transmission.
void chimera__chat_out(uint8_t channel)
{
    int32_t encoded_bits = message_delta_encode_message(0, 0xf, 0, (void **)&channel, 0, 1, 0);

    if (encoded_bits > 0) {
        uint8_t *session = *(uint8_t **)((uint8_t *)network_client + 0xadc);

        if ((session[0xa8c] & 1) == 0 &&
            (encoded_bits + 1 <= (*(int32_t *)(session + 0x24) + *(int32_t *)(session + 0x1c) * -8) -
                                      *(int32_t *)(session + 0x20) + 1 ||
             network_channel_stream_flush(session, 1) != 0)) {
            *(int32_t *)(session + 0xa80) = *(int32_t *)(session + 0xa80) + encoded_bits + 1;
            bit_stream_write_bits_chunked(1);
            session[0x2c] = 0;
            bit_stream_write_bits_chunked((uint32_t)encoded_bits);
            session[0x2c] = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4aab00):

void __cdecl chimera__chat_out(char channel)

{
  int iVar1;
  char cVar2;
  int iVar3;
  undefined3 uStack00000005;
  undefined1 local_c [4];
  char local_8;

  local_8 = channel;
  _channel = local_c;
  iVar3 = message_delta_encode_message(0,0xf,0,(void **)&channel,0,1,'\0');
  if (0 < iVar3) {
    iVar1 = *(int *)(DAT_0071c2d8 + 0xadc);
    _channel = (undefined1 *)CONCAT31(uStack00000005,1);
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
