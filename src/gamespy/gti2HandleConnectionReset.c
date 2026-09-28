// gti2HandleConnectionReset  (GameSpy SDK in halo.exe; no C existed)
// address 0x618870, size 138 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618870..0x6188f9: the receive dump callback (when set) hears a reset for the
//   address; a connection there still awaiting the server challenge fails with timed out (6) once its timeout (never
//   with 0) has passed; any later connection fails rejected (2); both as a remote close.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2HandleConnectionReset(GTI2Socket *socket, unsigned int ip, unsigned short port)
{
    GTI2Connection *connection = gti2SocketFindConnection(socket, ip, port);

    if (socket->receiveDumpCallback != 0 &&
        !gti2DumpCallback(socket, connection, ip, port, 1, 0, 0, 0, 0, 0)) {
        return 0;
    }
    if (connection == 0) {
        return 1;
    }
    if (connection->state == GTI2AwaitingServerChallenge) {
        if (connection->timeout == 0 || current_time() - connection->startTime < connection->timeout) {
            return 1;
        }
        return gti2ConnectionError(connection, 6, 1) != 0;
    }
    return gti2ConnectionError(connection, 2, 1) != 0;
}
