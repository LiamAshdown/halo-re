// gti2NewIncomingConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x61cd50, size 55 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61cd50..0x61cd86: gti2NewSocketConnection; a new one awaits the client
//   challenge (state 2), not initiated.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2NewIncomingConnection(GTI2Socket *socket, GTI2Connection **connection, unsigned int ip, unsigned short port)
{
    int result = gti2NewSocketConnection(socket, connection, ip, port);

    if (result != 0) {
        return result;
    }
    (*connection)->state = GTI2AwaitingClientChallenge;
    (*connection)->initiated = 0;
    return 0;
}
