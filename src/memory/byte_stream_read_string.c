// byte_stream_read_string  (Ghidra: byte_stream_read_string, already named)
// address 0x4d0930, size 70 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/memory_types_notes.md byte_stream section; matches byte_stream field
// layout exactly.
// register convention: byte_stream* in ECX (in_ECX); no other explicit registers.

#include "tags.h"
#include "memory.h"

// blam-cc: stream in ECX
// Reads a NUL-terminated string out of `stream` starting at the current cursor, returning its
// address in the stream's own buffer (not copied) and advancing the cursor past the terminating
// NUL. Flags overflow and returns NULL if no NUL is found before the stream's size limit.
char *byte_stream_read_string(byte_stream *stream)
{
    int32_t start;
    int32_t offset;
    int16_t length;

    start = stream->cursor;
    length = 0;
    if (start < stream->size) {
        offset = 0;
        do {
            if (stream->data[offset + start] == 0) {
                stream->cursor = length + 1 + start;
                return (char *)(stream->data + start);
            }
            length = length + 1;
            offset = (int32_t)length;
        } while (start + offset < stream->size);
    }
    stream->overflow = 1;
    return 0;
}

#if 0
Original Ghidra decompilation (0x4d0930):

int byte_stream_read_string(void)

{
  int iVar1;
  int *in_ECX;
  int iVar2;
  short sVar3;

  iVar1 = in_ECX[1];
  sVar3 = 0;
  if (iVar1 < in_ECX[2]) {
    iVar2 = 0;
    do {
      if (*(char *)(iVar2 + *in_ECX + iVar1) == '\0') {
        in_ECX[1] = sVar3 + 1 + iVar1;
        return *in_ECX + iVar1;
      }
      sVar3 = sVar3 + 1;
      iVar2 = (int)sVar3;
    } while (in_ECX[1] + iVar2 < in_ECX[2]);
  }
  *(undefined1 *)(in_ECX + 3) = 1;
  return 0;
}
#endif
