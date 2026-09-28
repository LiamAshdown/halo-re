// ghiInitFixedBuffer  (GameSpy SDK in halo.exe; no C existed)
// address 0x622b50, size 75 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622b50..0x622b9a: wraps the caller  buffer: fixed, never freed, empty (pos is
//   left as it was).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiInitFixedBuffer(GHIConnection *connection, GHIBuffer *buffer, char *userBuffer, int size)
{
    if (connection == 0 || buffer == 0 || userBuffer == 0 || size <= 0) {
        return 0;
    }
    buffer->size = size;
    buffer->connection = connection;
    buffer->data = userBuffer;
    buffer->len = 0;
    buffer->sizeIncrement = 0;
    buffer->fixed = 1;
    buffer->dontFree = 1;
    userBuffer[0] = 0;
    return 1;
}
