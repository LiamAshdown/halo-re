// gti2SocketError  (GameSpy SDK in halo.exe; no C existed)
// address 0x61cba0, size 49 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61cba0..0x61cbd0: once per socket: marks the error, hard-closes all connections
//   (gt2CloseAllConnectionsHard 0x614780) and, when the error callback says to go on, frees the socket.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

extern void gt2CloseAllConnectionsHard(GTI2Socket *socket);

void gti2SocketError(GTI2Socket *socket)
{
    if (socket->error != 0) {
        return;
    }
    socket->error = 1;
    gt2CloseAllConnectionsHard(socket);
    if (gti2SocketErrorCallback(socket)) {
        gti2FreeSocket(socket);
    }
}
