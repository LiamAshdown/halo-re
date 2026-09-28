// ghiCallProgressCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x620da0, size 53 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620da0..0x620dd4: the progress callback (request, state, buffer, len, bytes
//   received, total size, param).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiCallProgressCallback(GHIConnection *connection, const char *buffer, int bufferLen)
{
    if (connection->progressCallback != 0) {
        connection->progressCallback(connection->request, connection->state, buffer, bufferLen,
            connection->fileBytesReceived, connection->totalSize, connection->callbackParam);
    }
}
