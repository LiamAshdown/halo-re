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

extern uint8_t message_delta_parameters_enabled;           // 0x0071cfa8
extern uint8_t message_delta_parameters_sending;            // 0x0071cfb4

extern uint8_t bit_stream_write_bit(int32_t bit_value, bit_stream *stream); // UNSURE: stream arg
extern int32_t bit_stream_write_bits_chunked(int32_t total_bit_count, uint32_t value, bit_stream *stream); // UNSURE: args

// Writes the leading header bit(s) of a message-delta message: whether the message is
// incremental, then (under protocol v2) the parameters-in-progress bit, using the encode
// context's own scratch counters at +0x80..+0x88 as running bit-position state.
char message_delta_encode_message_header(uint8_t *ctx)
{
    #define CTXD(off) (*(int32_t *)(ctx + (off)))
    bit_stream *stream = *(bit_stream **)(ctx + 0xc); // UNSURE: best-guess stream slot, see header
    char header_ok;
    char v2_ok;
    int32_t written;

    header_ok = (char)bit_stream_write_bit(CTXD(8), stream);
    CTXD(0x84) = CTXD(0x84) + 6;
    CTXD(0x80) = 1;
    written = bit_stream_write_bits_chunked(CTXD(0x84), 0, stream); // UNSURE: value argument
    if (written == 0 || header_ok == 0) {
        header_ok = 0;
    } else {
        header_ok = 1;
    }
    if (message_delta_parameters_enabled != 1) {
        return header_ok;
    }
    CTXD(0x88) = 2;
    v2_ok = (char)bit_stream_write_bit(message_delta_parameters_sending, stream);
    written = bit_stream_write_bits_chunked(CTXD(0x88), 0, stream); // UNSURE: value argument
    if (written != 0 && v2_ok != 0 && header_ok != 0) {
        CTXD(0x88) = CTXD(0x88) + 1;
        return 1;
    }
    CTXD(0x88) = CTXD(0x88) + 1;
    return 0;
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
