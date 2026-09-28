// gt2Think  (GameSpy SDK in halo.exe; no C existed)
// address 0x614540, size 42 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614540..0x614569: receives and handles everything waiting on the socket, then
//   lets every connection think; when both succeed the closed connections are freed.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

void gt2Think(GTI2Socket *socket)
{
    if (gti2ReceiveMessages(socket) && gti2SocketConnectionsThink(socket)) {
        gti2FreeClosedConnections(socket);
    }
}
