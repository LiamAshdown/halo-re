// gti2AllocateBuffer  (GameSpy SDK in halo.exe; no C existed)
// address 0x620520, size 36 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620520..0x620543: mallocs the buffer data; 0 when out of memory, else the size
//   is set and 1.
// blam-cc: cdecl

#include "gamespy.h"

typedef struct GTI2Buffer {
    unsigned char *buffer;            // 0x0
    int size;                         // 0x4
    int len;                          // 0x8
} GTI2Buffer;

int gti2AllocateBuffer(GTI2Buffer *buffer, int size)
{
    buffer->buffer = (unsigned char *)malloc(size);
    if (buffer->buffer == 0) {
        return 0;
    }
    buffer->size = size;
    return 1;
}
