// gti2HandleClose  (GameSpy SDK in halo.exe; no C existed)
// address 0x6190a0, size 68 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6190a0..0x6190e3: EAX connection: answers with closed, then an error (2; reason
//   local close when we were closing, else remote close).
// blam-cc: EAX -> connection

#include "gamespy.h"

#include "gt2.h"

int gti2HandleClose(GTI2Connection *connection)
{
    if (!gti2SendClosed(connection->socket, connection->ip, connection->port)) {
        return 0;
    }
    return gti2ConnectionError(connection, 2, connection->state != GTI2Closing) != 0;
}
