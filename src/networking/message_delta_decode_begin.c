// message_delta_decode_begin  (Ghidra: FUN_004ec490; named per this rewrite)
// address 0x4ec490, size 120 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: objdump -d -M intel bin/halo.exe @0x4ec490: `esi = eax` (the decode_state output
// pointer), a genuine `push edi` / `call 0x4ece70` (message_delta_decode_message_header) whose
// only recognized parameter is the stream, and on success writes decode_state.bits_read (+0xc),
// .start_bit_offset (+0x14), .stream (+0x10) and .processed_count (+0x18) exactly as
// types/networking.h documents message_delta_decode_state. message_delta_decode_message_header
// itself reads and writes the same decode_state through ESI, which is only valid because this
// function leaves it live in that register across the call (x86 cdecl callees preserve ESI) --
// exposed here as an explicit second parameter instead of an implicit register pin.
// register convention: decode_state output in EAX (recognized parameter), stream in EDI
// (unaff_EDI).
// blam-cc: EAX -> state, EDI -> stream

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t message_delta_decode_message_header(bit_stream *stream, message_delta_decode_state *state); // 0x4ece70, this module

// blam-cc: EAX -> state, EDI -> stream
// Begins decoding a message-delta message: decodes the header into *state and, on success,
// seeds the rest of the decode state (bits_read, stream, processed_count) for the field-decode
// loop that follows. On failure, rewinds the stream to where it started and marks state as
// stateless (baseline).
int32_t message_delta_decode_begin(message_delta_decode_state *state, bit_stream *stream)
{
    int32_t initial_offset;
    int32_t header_bits;
    uint32_t target;

    initial_offset = (int32_t)(stream->bit_cursor + stream->byte_cursor * 8) - (int32_t)stream->first_bit;
    header_bits = message_delta_decode_message_header(stream, state);
    if (0 < header_bits) {
        state->bits_read = header_bits;
        state->start_bit_offset = initial_offset;
        state->stream = stream;
        state->processed_count = 1;
        return 1;
    }
    state->incremental = 0;
    target = (uint32_t)stream->first_bit + (uint32_t)initial_offset;
    if ((initial_offset >= 0 || target <= stream->first_bit) &&
        (initial_offset <= 0 || stream->first_bit <= target) &&
        ((stream->first_bit <= target && target <= stream->last_bit) || target == stream->last_bit + 1)) {
        stream->bit_cursor = target & 7;
        stream->byte_cursor = target >> 3;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ec490):

uint FUN_004ec490(void)

{
  undefined4 *in_EAX;
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int unaff_EDI;

  iVar1 = (*(int *)(unaff_EDI + 0x10) + *(int *)(unaff_EDI + 0xc) * 8) - *(int *)(unaff_EDI + 8);
  iVar2 = message_delta_decode_message_header();
  if (iVar2 < 1) {
    *in_EAX = 0;
    uVar4 = *(uint *)(unaff_EDI + 8);
    uVar3 = uVar4 + iVar1;
    if ((((-1 < iVar1) || (uVar3 <= uVar4)) && ((iVar1 < 1 || (uVar4 <= uVar3)))) &&
       (((uVar4 <= uVar3 && (uVar3 <= *(uint *)(unaff_EDI + 0x14))) ||
        (uVar3 == *(int *)(unaff_EDI + 0x14) + 1U)))) {
      uVar4 = uVar3 & 7;
      uVar3 = uVar3 >> 3;
      *(uint *)(unaff_EDI + 0x10) = uVar4;
      *(uint *)(unaff_EDI + 0xc) = uVar3;
    }
    return uVar3 & 0xffffff00;
  }
  in_EAX[3] = iVar2;
  in_EAX[5] = iVar1;
  in_EAX[4] = unaff_EDI;
  in_EAX[6] = 1;
  return 1;
}
#endif
