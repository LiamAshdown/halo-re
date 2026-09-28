// ghiCallCompletedCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x620d50, size 78 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620d50..0x620d9d: the completed callback (request, result, and for a get the
//   file buffer and bytes received); a false answer keeps the buffer (not freed).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiCallCompletedCallback(GHIConnection *connection)
{
    char *buffer;
    int len;

    if (connection->completedCallback == 0) {
        return;
    }
    if (connection->type == 0) {
        buffer = connection->getFileBuffer.data;
        len = connection->fileBytesReceived;
    } else {
        buffer = 0;
        len = 0;
    }
    if (!connection->completedCallback(connection->request, connection->result, buffer, len, connection->callbackParam) &&
        buffer != 0) {
        connection->getFileBuffer.dontFree = 1;
    }
}
