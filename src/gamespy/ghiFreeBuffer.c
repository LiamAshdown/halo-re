// ghiFreeBuffer  (GameSpy SDK in halo.exe; no C existed)
// address 0x622ba0, size 59 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622ba0..0x622bda: frees the data unless marked not to, and clears the buffer.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiFreeBuffer(GHIBuffer *buffer)
{
    if (buffer == 0 || buffer->data == 0) {
        return;
    }
    if (buffer->dontFree == 0) {
        free(buffer->data);
    }
    memset(buffer, 0, sizeof(*buffer));
}
