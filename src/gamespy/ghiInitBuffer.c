// ghiInitBuffer  (GameSpy SDK in halo.exe; no C existed)
// address 0x622af0, size 90 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622af0..0x622b49: an empty growable buffer of the initial size (NUL at 0); 0
//   for bad arguments or no memory.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiInitBuffer(GHIConnection *connection, GHIBuffer *buffer, int initialSize, int sizeIncrement)
{
    if (connection == 0 || buffer == 0 || initialSize <= 0 || sizeIncrement <= 0) {
        return 0;
    }
    buffer->connection = connection;
    buffer->data = 0;
    buffer->size = 0;
    buffer->len = 0;
    buffer->pos = 0;
    buffer->sizeIncrement = sizeIncrement;
    buffer->fixed = 0;
    buffer->dontFree = 0;
    if (!ghiResizeBuffer(buffer, initialSize)) {
        return 0;
    }
    buffer->data[0] = 0;
    return 1;
}
