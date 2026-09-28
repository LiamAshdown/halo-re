// gti2HandleOutOfMemory  (GameSpy SDK in halo.exe; no C existed)
// address 0x618ee0, size 45 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618ee0..0x618f0c: EAX connection: tells the peer it is closed, then an error:
//   out of memory (1), not enough memory (4).
// blam-cc: EAX -> connection

#include "gamespy.h"

#include "gt2.h"

int gti2HandleOutOfMemory(GTI2Connection *connection)
{
    if (!gti2SendClosed(connection->socket, connection->ip, connection->port)) {
        return 0;
    }
    return gti2ConnectionError(connection, 1, 4);
}
