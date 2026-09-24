// byte_stream_write_ranged_integer  (Ghidra: FUN_004d0700)
// address 0x4d0700, size 217 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/memory_types_notes.md "FUN_004d0700 (byte_stream_write_ranged_integer),
// byte_stream_write_string @0x4d07e0, byte_stream_read_long @0x4d0850, FUN_004d08a0,
// byte_stream_read_string @0x4d0930 all use ESI/ECX[0] = buffer, [1] = cursor, [2] = size, and
// set the low byte of [3] to 1 on overflow."; matches types/memory.h byte_stream exactly.
// register convention: maximum-value selector in EAX (in_EAX), value to write in EDX (in_EDX),
// byte_stream* in ESI (unaff_ESI).

#include "tags.h"
#include "memory.h"

extern void byte_swap_array(int32_t size_code, uint32_t *array, int32_t count); // blam-cc: size code EAX, array ECX, count EDX

// blam-cc: maximum in EAX, value in EDX, stream in ESI
// Writes `value` into `stream`, using the narrowest of 1/2/4 bytes that can represent `maximum`
// (matching the encoding struct_definition_encode uses for a ranged/counted field), stored
// big-endian. Sets the stream's overflow flag and returns false if there isn't room; returns
// true (the overflow flag was and remains clear) on success.
uint32_t byte_stream_write_ranged_integer(int32_t maximum, uint32_t value, byte_stream *stream)
{
    uint8_t *dst;
    uint16_t native16;

    if (maximum < 0x100) {
        if (stream->cursor + 1 <= stream->size && stream->overflow == 0) {
            *(stream->data + stream->cursor) = (uint8_t)value;
            stream->cursor = stream->cursor + 1;
            return stream->overflow == 0;
        }
    } else if (maximum < 0x10000) {
        if (stream->cursor + 2 <= stream->size && stream->overflow == 0) {
            dst = stream->data + stream->cursor;
            native16 = (uint16_t)value;
            *(uint16_t *)dst = native16;
            *(uint16_t *)dst = (uint16_t)(((value & 0xff) << 8) | ((value >> 8) & 0xff));
            stream->cursor = stream->cursor + 2;
            return stream->overflow == 0;
        }
    } else if (stream->cursor + 4 <= stream->size && stream->overflow == 0) {
        *(uint32_t *)(stream->data + stream->cursor) = value;
        byte_swap_array(-4, (uint32_t *)(stream->data + stream->cursor), 1);
        stream->cursor = stream->cursor + 4;
        return stream->overflow == 0;
    }
    stream->overflow = 1;
    return stream->overflow == 0;
}

#if 0
Original Ghidra decompilation (0x4d0700):

bool FUN_004d0700(void)

{
  int in_EAX;
  undefined2 *puVar1;
  undefined4 in_EDX;
  int *unaff_ESI;
  undefined2 local_4;

  if (in_EAX < 0x100) {
    if ((unaff_ESI[1] + 1 <= unaff_ESI[2]) && ((char)unaff_ESI[3] == '\0')) {
      *(char *)(*unaff_ESI + unaff_ESI[1]) = (char)in_EDX;
      unaff_ESI[1] = unaff_ESI[1] + 1;
      return (char)unaff_ESI[3] == '\0';
    }
  }
  else if (in_EAX < 0x10000) {
    if ((unaff_ESI[1] + 2 <= unaff_ESI[2]) && ((char)unaff_ESI[3] == '\0')) {
      puVar1 = (undefined2 *)(*unaff_ESI + unaff_ESI[1]);
      local_4 = (undefined2)in_EDX;
      *puVar1 = local_4;
      *puVar1 = CONCAT11((char)in_EDX,(char)((uint)in_EDX >> 8));
      unaff_ESI[1] = unaff_ESI[1] + 2;
      return (char)unaff_ESI[3] == '\0';
    }
  }
  else if ((unaff_ESI[1] + 4 <= unaff_ESI[2]) && ((char)unaff_ESI[3] == '\0')) {
    *(undefined4 *)(*unaff_ESI + unaff_ESI[1]) = in_EDX;
    byte_swap_array();
    unaff_ESI[1] = unaff_ESI[1] + 4;
    return (char)unaff_ESI[3] == '\0';
  }
  *(undefined1 *)(unaff_ESI + 3) = 1;
  return (char)unaff_ESI[3] == '\0';
}
#endif
