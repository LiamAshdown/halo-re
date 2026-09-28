// ghiAppendHeaderToBuffer  (GameSpy SDK in halo.exe; no C existed)
// address 0x622cd0, size 93 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622cd0..0x622d2c: name ": " value CRLF.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiAppendHeaderToBuffer(GHIBuffer *buffer, const char *name, const char *value)
{
    return ghiAppendDataToBuffer(buffer, name, 0) && ghiAppendDataToBuffer(buffer, ": ", 2) &&
           ghiAppendDataToBuffer(buffer, value, 0) && ghiAppendDataToBuffer(buffer, "\r\n", 2);
}
