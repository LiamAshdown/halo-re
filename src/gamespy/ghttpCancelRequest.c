// ghttpCancelRequest  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c030, size 27 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c030..0x61c04a: frees the request  connection when it is live.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghttpCancelRequest(int request)
{
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection != 0) {
        ghiFreeConnection(connection);
    }
}
