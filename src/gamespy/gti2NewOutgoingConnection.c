// gti2NewOutgoingConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x61cd10, size 55 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61cd10..0x61cd46: gti2NewSocketConnection; a new one awaits the server
//   challenge (state 0) and is initiated.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2NewOutgoingConnection(GTI2Socket *socket, GTI2Connection **connection, unsigned int ip, unsigned short port)
{
    int result = gti2NewSocketConnection(socket, connection, ip, port);

    if (result != 0) {
        return result;
    }
    (*connection)->state = GTI2AwaitingServerChallenge;
    (*connection)->initiated = 1;
    return 0;
}
