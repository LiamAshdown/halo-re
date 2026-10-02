// byte_stream_read_long  (Ghidra: byte_stream_read_long, already named)
// address 0x4d0850, size 67 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/memory_types_notes.md byte_stream section; callee byte_swap_array
// (endian conversion, matching byte_stream_write_ranged_integer's own use of it).
// register convention: byte_stream* in ESI (unaff_ESI); no other explicit registers.

#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void byte_swap_array(int32_t size_code, uint32_t *array, int32_t count); // blam-cc: size code EAX, array ECX, count EDX

// blam-cc: stream in ESI
// Reads and endian-swaps a 4-byte big-endian value out of `stream`, advancing the cursor.
// Flags overflow and returns 0 if the stream is exhausted (or already overflowed).
uint32_t byte_stream_read_long(byte_stream *stream)
{
    uint32_t *value_ptr;

    if (stream->size < stream->cursor + 4 || stream->overflow != 0) {
        stream->overflow = 1;
    } else {
        value_ptr = (uint32_t *)(stream->data + stream->cursor);
        byte_swap_array(-4, value_ptr, 1);
        stream->cursor = stream->cursor + 4;
        if (value_ptr != 0) {
            return *value_ptr;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4d0850):

undefined4 byte_stream_read_long(void)

{
  int *unaff_ESI;
  undefined4 *puVar1;

  if ((unaff_ESI[2] < unaff_ESI[1] + 4) || ((char)unaff_ESI[3] != '\0')) {
    *(undefined1 *)(unaff_ESI + 3) = 1;
  }
  else {
    puVar1 = (undefined4 *)(*unaff_ESI + unaff_ESI[1]);
    byte_swap_array();
    unaff_ESI[1] = unaff_ESI[1] + 4;
    if (puVar1 != (undefined4 *)0x0) {
      return *puVar1;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
