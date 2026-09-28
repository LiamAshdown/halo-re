// gti2BufferWriteData  (GameSpy SDK in halo.exe; no C existed)
// address 0x6205b0, size 82 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6205b0..0x620601: appends length bytes (strlen for -1); nothing for NULL data
//   or 0 bytes.
// blam-cc: cdecl

#include "gamespy.h"

typedef struct GTI2Buffer {
    unsigned char *buffer;            // 0x0
    int size;                         // 0x4
    int len;                          // 0x8
} GTI2Buffer;

void gti2BufferWriteData(GTI2Buffer *buffer, const unsigned char *data, int length)
{
    if (data == 0 || length == 0) {
        return;
    }
    if (length == -1) {
        length = (int)strlen((const char *)data);
    }
    memcpy(buffer->buffer + buffer->len, data, (unsigned int)length);
    buffer->len += length;
}
