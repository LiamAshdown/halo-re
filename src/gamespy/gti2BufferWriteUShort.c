// gti2BufferWriteUShort  (GameSpy SDK in halo.exe; no C existed)
// address 0x620580, size 35 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620580..0x6205a2: appends a short, high byte first.
// blam-cc: cdecl

#include "gamespy.h"

typedef struct GTI2Buffer {
    unsigned char *buffer;            // 0x0
    int size;                         // 0x4
    int len;                          // 0x8
} GTI2Buffer;

void gti2BufferWriteUShort(GTI2Buffer *buffer, unsigned short s)
{
    buffer->buffer[buffer->len] = (unsigned char)(s >> 8);
    buffer->len++;
    buffer->buffer[buffer->len] = (unsigned char)s;
    buffer->len++;
}
