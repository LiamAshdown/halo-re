// gti2BufferShorten  (GameSpy SDK in halo.exe; no C existed)
// address 0x620610, size 60 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620610..0x62064b: removes shortenBy bytes at start (-1: from the end) by moving
//   the rest down.
// blam-cc: cdecl

#include "gamespy.h"

typedef struct GTI2Buffer {
    unsigned char *buffer;            // 0x0
    int size;                         // 0x4
    int len;                          // 0x8
} GTI2Buffer;

void gti2BufferShorten(GTI2Buffer *buffer, int start, int shortenBy)
{
    if (start == -1) {
        start = buffer->len - shortenBy;
    }
    memmove(buffer->buffer + start, buffer->buffer + start + shortenBy, buffer->len - start - shortenBy);
    buffer->len -= shortenBy;
}
