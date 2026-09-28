// ghiDoWaiting  (GameSpy SDK in halo.exe; no C existed)
// address 0x621330, size 87 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621330..0x621386: readable: receiving the status (5); select failure fails with
//   socket failed (5).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiDoWaiting(GHIConnection *connection)
{
    int readFlag;

    if (!ghiSocketSelect(connection->socket, &readFlag, 0, 0)) {
    connection->completed = 1;
    connection->result = 5;
        connection->socketError = WSAGetLastError();
        return;
    }
    if (readFlag != 0) {
        connection->state = 5;
    ghiCallProgressCallback(connection, 0, 0);
    }
}
