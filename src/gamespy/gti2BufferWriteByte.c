// gti2BufferWriteByte  (GameSpy SDK in halo.exe; no C existed)
// address 0x620560, size 22 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620560..0x620575: appends one byte.
// blam-cc: cdecl

#include "gamespy.h"

typedef struct GTI2Buffer {
    unsigned char *buffer;            // 0x0
    int size;                         // 0x4
    int len;                          // 0x8
} GTI2Buffer;

void gti2BufferWriteByte(GTI2Buffer *buffer, unsigned char b)
{
    buffer->buffer[buffer->len] = b;
    buffer->len++;
}
