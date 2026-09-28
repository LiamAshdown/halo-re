// ghiCallPostCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x620de0, size 71 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620de0..0x620e26: the post callback (request, bytes posted, total bytes,
//   objects posted, object count) with the connection  callback param (not the post  own).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiCallPostCallback(GHIConnection *connection)
{
    if (connection->postCallback != 0) {
        connection->postCallback(connection->request, connection->bytesPosted, connection->totalBytes,
            connection->objectsPosted, ArrayLength(connection->postingStates), connection->callbackParam);
    }
}
