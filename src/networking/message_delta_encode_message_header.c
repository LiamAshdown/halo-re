// message_delta_encode_message_header  (Ghidra: message_delta_encode_message_header, already named)
// address 0x4ecd00, size 210 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: objdump -d -M intel bin/halo.exe @0x4ec997/0x4ec9c8: message_delta_encode_message
// leaves the encode context's address live in ESI from its own setup all the way to this call,
// matching the same context base FUN_004ecb60 (0x4ecb60) receives via EAX.
// register convention: encode context pinned in ESI (unaff_ESI).
// UNSURE: see message_delta_encode_prepare_item.c -- the encode context is only partly resolved;
// fields +0x80..+0x88 are used solely by this function and are not cross-referenced elsewhere.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_memory.h"

extern uint8_t message_delta_parameters_enabled;           // 0x0071cfa8
extern uint8_t message_delta_parameters_sending;            // 0x0071cfb4

extern uint8_t bit_stream_write_bit(uint8_t bit, bit_stream *stream); // 0x4cf9a0, EDX stream, stack bit

extern int32_t message_delta_parameters_protocol_sequence; // 0x0071cfac, written as the 2-bit parameter field

// Writes the leading header bit(s) of a message-delta message: whether the message is
// incremental, then (under protocol v2) the parameters-in-progress bit, using the encode
// context's own scratch counters at +0x80..+0x88 as running bit-position state.
// REWRITTEN from objdump 0x4ecd00..0x4ecdd1. ESI is the encoder context; its bit stream is INLINE at ctx+0x1c (the draft
// read a pointer at +0xc). Writes: the flag bit (ctx+8); the message type -- the dword at ctx+4, passed by address --
// in ctx+0x84 + 6 bits (ctx+0x84 updated, ctx+0x80 = 1); and, when message-delta parameters are enabled, the "sending"
// bit and the 2-bit parameter field from 0x71cfac (ctx+0x88 = 2, then incremented). Returns AL: every write succeeded.
// blam-cc: ESI -> ctx
uint8_t message_delta_encode_message_header(uint8_t *ctx)
{
    #define CTXD(off) (*(int32_t *)(ctx + (off)))
    bit_stream *stream = (bit_stream *)(ctx + 0x1c);
    uint32_t parameters = (uint32_t)message_delta_parameters_protocol_sequence;   // 0x4ecd01: read before anything else
    uint8_t ok;
    int32_t written;

    ok = bit_stream_write_bit((uint8_t)CTXD(8), stream) != 0;
    CTXD(0x80) = 1;
    CTXD(0x84) = CTXD(0x84) + 6;
    written = bit_stream_write_bits_chunked(stream, (const uint32_t *)(ctx + 4), CTXD(0x84));
    ok = (written != 0 && ok) ? 1 : 0;
    if (message_delta_parameters_enabled != 1) {
        return ok;
    }
    CTXD(0x88) = 2;
    ok = (bit_stream_write_bit(message_delta_parameters_sending, stream) != 0 && ok) ? 1 : 0;
    written = bit_stream_write_bits_chunked(stream, &parameters, CTXD(0x88));
    CTXD(0x88) = CTXD(0x88) + 1;
    return (written != 0 && ok) ? 1 : 0;
    #undef CTXD
}

#if 0
Original Ghidra decompilation (0x4ecd00):

char message_delta_encode_message_header(void)

{
  bool bVar1;
  undefined1 uVar2;
  char cVar3;
  char cVar4;
  int iVar5;
  int unaff_ESI;

  cVar3 = bit_stream_write_bit(*(undefined4 *)(unaff_ESI + 8));
  iVar5 = *(int *)(unaff_ESI + 0x84) + 6;
  *(undefined4 *)(unaff_ESI + 0x80) = 1;
  *(int *)(unaff_ESI + 0x84) = iVar5;
  iVar5 = FUN_004cf8f0(iVar5);
  uVar2 = DAT_0071cfb4;
  if ((iVar5 == 0) || (cVar3 == '\0')) {
    cVar3 = '\0';
  }
  else {
    cVar3 = '\x01';
  }
  if (DAT_0071cfa8 != '\x01') {
    return cVar3;
  }
  *(undefined4 *)(unaff_ESI + 0x88) = 2;
  cVar4 = bit_stream_write_bit(uVar2);
  if ((cVar4 == '\0') || (cVar3 == '\0')) {
    bVar1 = false;
  }
  else {
    bVar1 = true;
  }
  iVar5 = FUN_004cf8f0(*(undefined4 *)(unaff_ESI + 0x88));
  if ((iVar5 != 0) && (bVar1)) {
    *(int *)(unaff_ESI + 0x88) = *(int *)(unaff_ESI + 0x88) + 1;
    return '\x01';
  }
  *(int *)(unaff_ESI + 0x88) = *(int *)(unaff_ESI + 0x88) + 1;
  return '\0';
}
#endif
