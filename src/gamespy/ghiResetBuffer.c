// ghiResetBuffer  (GameSpy SDK in halo.exe; no C existed)
// address 0x622df0, size 18 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622df0..0x622e01: empty again (len, pos 0, NUL).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiResetBuffer(GHIBuffer *buffer)
{
    buffer->len = 0;
    buffer->pos = 0;
    buffer->data[0] = 0;
}
