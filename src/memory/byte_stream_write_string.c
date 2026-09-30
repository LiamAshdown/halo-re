// byte_stream_write_string  (Ghidra: byte_stream_write_string, already named)
// address 0x4d07e0, size 112 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: out/phase4/memory_types_notes.md byte_stream section; matches byte_stream field
// layout exactly (data, cursor, size, overflow).
// register convention: string pointer as the recognized parameter (param_1); maximum length in
// CX (in_CX); byte_stream* in ESI (unaff_ESI).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "fn_memory.h"


// blam-cc: max_length in CX, stream in ESI, string as the recognized parameter
// Writes a NUL-terminated copy of `string` into `stream`, capped to the first max_length
// characters (or up to the first NUL, whichever comes first). Sets the overflow flag and returns
// false if there isn't room for the string plus its NUL; returns true on success.
uint32_t byte_stream_write_string(char *string, int16_t max_length, byte_stream *stream)
{
    int32_t length;
    char *scan;
    char ch;
    int32_t count;
    char *dst;

    length = 0;
    scan = string;
    if (0 < max_length) {
        do {
            ch = *scan;
            scan = scan + 1;
            if (ch == 0) {
                break;
            }
            length = length + 1;
        } while (length < max_length);
    }
    count = (int16_t)length;
    dst = (char *)(stream->data + stream->cursor);
    if (count + 1 + stream->cursor <= stream->size && stream->overflow == 0) {
        strncpy(dst, string, (uint32_t)count);
        dst[count] = 0;
        stream->cursor = stream->cursor + count + 1;
        return stream->overflow == 0;
    }
    stream->overflow = 1;
    return stream->overflow == 0;
}

#if 0
Original Ghidra decompilation (0x4d07e0):

bool byte_stream_write_string(char *param_1)

{
  char cVar1;
  int iVar2;
  short in_CX;
  size_t _Count;
  int *unaff_ESI;
  char *pcVar3;

  iVar2 = 0;
  pcVar3 = param_1;
  if (0 < in_CX) {
    do {
      cVar1 = *pcVar3;
      pcVar3 = pcVar3 + 1;
      if (cVar1 == '\0') break;
      iVar2 = iVar2 + 1;
    } while (iVar2 < in_CX);
  }
  _Count = (size_t)(short)iVar2;
  pcVar3 = (char *)(*unaff_ESI + unaff_ESI[1]);
  if (((int)(_Count + 1 + unaff_ESI[1]) <= unaff_ESI[2]) && ((char)unaff_ESI[3] == '\0')) {
    _strncpy(pcVar3,param_1,_Count);
    pcVar3[_Count] = '\0';
    unaff_ESI[1] = unaff_ESI[1] + _Count + 1;
    return (char)unaff_ESI[3] == '\0';
  }
  *(undefined1 *)(unaff_ESI + 3) = 1;
  return (char)unaff_ESI[3] == '\0';
}
#endif
