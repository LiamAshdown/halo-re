// gti2GetBufferFreeSpace  (GameSpy SDK in halo.exe; no C existed)
// address 0x620550, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620550..0x62055a: size - len.
// blam-cc: cdecl

#include "gamespy.h"

typedef struct GTI2Buffer {
    unsigned char *buffer;            // 0x0
    int size;                         // 0x4
    int len;                          // 0x8
} GTI2Buffer;

int gti2GetBufferFreeSpace(const GTI2Buffer *buffer)
{
    return buffer->size - buffer->len;
}
