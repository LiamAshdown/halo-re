// ghiAppendCharToBuffer  (GameSpy SDK in halo.exe; no C existed)
// address 0x622d30, size 120 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622d30..0x622da7: one character and a NUL; growing by one increment when full
//   (fixed: overflow 2; no memory: 1).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiAppendCharToBuffer(GHIBuffer *buffer, int c)
{
    if (buffer == 0) {
        return 0;
    }
    if (buffer->len + 1 >= buffer->size) {
        if (buffer->fixed != 0) {
            buffer->connection->completed = 1;
            buffer->connection->result = 2;
            return 0;
        }
        if (!ghiResizeBuffer(buffer, buffer->sizeIncrement)) {
            buffer->connection->completed = 1;
            buffer->connection->result = 1;
            return 0;
        }
    }
    buffer->data[buffer->len] = (char)c;
    buffer->len++;
    buffer->data[buffer->len] = 0;
    return 1;
}
