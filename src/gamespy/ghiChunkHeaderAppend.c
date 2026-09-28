// ghiChunkHeaderAppend  (GameSpy SDK in halo.exe; no C existed)
// address 0x621620, size 90 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621620..0x621679: EAX connection, EDX length, stack data: keeps up to 10 chunk-
//   header characters (NUL-terminated).
// blam-cc: EAX -> connection, EDX -> len, stack -> buffer

#include "gamespy.h"

#include "ghttp.h"

void ghiChunkHeaderAppend(GHIConnection *connection, const char *buffer, int len)
{
    if (len == 0 || connection->chunkHeaderLen >= 10) {
        return;
    }
    if (10 - connection->chunkHeaderLen < len) {
        len = 10 - connection->chunkHeaderLen;
    }
    memcpy(connection->chunkHeader + connection->chunkHeaderLen, buffer, len);
    connection->chunkHeaderLen += len;
    connection->chunkHeader[connection->chunkHeaderLen] = 0;
}
