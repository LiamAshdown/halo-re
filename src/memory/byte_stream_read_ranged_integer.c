// byte_stream_read_ranged_integer  (Ghidra: FUN_004d08a0)
// address 0x4d08a0, size 133 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/memory_types_notes.md "FUN_004d08a0 ... delegating to
// byte_stream_read_long for the widest case."; mirror of byte_stream_write_ranged_integer
// (0x4d0700), reading instead of writing.
// register convention: maximum-value selector in EAX (in_EAX), byte_stream* in ECX (in_ECX).
// UNSURE: the >0xffff case calls byte_stream_read_long() with no explicit argument in the
// Ghidra output; this only works if ESI already equals the same stream pointer this function
// received in ECX (byte_stream_read_long's own convention is stream-in-ESI). Preserved here as
// an ordinary call passing `stream` explicitly, since that is the only value it could
// correctly be.

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"


// blam-cc: maximum in EAX, stream in ECX
// Reads a value out of `stream`, using the narrowest of 1/2/4 bytes that can represent `maximum`
// (the inverse of byte_stream_write_ranged_integer), stored big-endian. Flags overflow and
// returns 0 if there isn't room.
uint32_t byte_stream_read_ranged_integer(int32_t maximum, byte_stream *stream)
{
    int32_t next_cursor;
    uint8_t *byte_ptr;
    uint16_t *word_ptr;

    if (maximum < 0x100) {
        next_cursor = stream->cursor + 1;
        if (stream->size < next_cursor || stream->overflow != 0) {
            stream->overflow = 1;
        } else {
            byte_ptr = stream->data + stream->cursor;
            stream->cursor = next_cursor;
            if (byte_ptr != 0) {
                return (uint32_t)*byte_ptr;
            }
        }
        return 0;
    }
    if (0xffff < maximum) {
        return byte_stream_read_long(stream);
    }
    if (stream->cursor + 2 <= stream->size && stream->overflow == 0) {
        word_ptr = (uint16_t *)(stream->data + stream->cursor);
        *word_ptr = (uint16_t)(((*word_ptr & 0xff) << 8) | ((*word_ptr >> 8) & 0xff));
        stream->cursor = stream->cursor + 2;
        return (uint32_t)(int16_t)*word_ptr;
    }
    stream->overflow = 1;
    return 0;
}

#if 0
Original Ghidra decompilation (0x4d08a0):

uint FUN_004d08a0(void)

{
  int iVar1;
  int in_EAX;
  byte *pbVar2;
  short *psVar3;
  uint uVar4;
  int *in_ECX;

  if (in_EAX < 0x100) {
    iVar1 = in_ECX[1] + 1;
    if ((in_ECX[2] < iVar1) || ((char)in_ECX[3] != '\0')) {
      *(undefined1 *)(in_ECX + 3) = 1;
    }
    else {
      pbVar2 = (byte *)(*in_ECX + in_ECX[1]);
      in_ECX[1] = iVar1;
      if (pbVar2 != (byte *)0x0) {
        return (uint)*pbVar2;
      }
    }
    return 0;
  }
  if (0xffff < in_EAX) {
    uVar4 = byte_stream_read_long();
    return uVar4;
  }
  if ((in_ECX[1] + 2 <= in_ECX[2]) && ((char)in_ECX[3] == '\0')) {
    psVar3 = (short *)(*in_ECX + in_ECX[1]);
    *psVar3 = CONCAT11((char)*psVar3,(char)((ushort)*psVar3 >> 8));
    in_ECX[1] = in_ECX[1] + 2;
    return (int)*psVar3;
  }
  *(undefined1 *)(in_ECX + 3) = 1;
  return 0;
}
#endif
