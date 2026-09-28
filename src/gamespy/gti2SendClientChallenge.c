// gti2SendClientChallenge  (GameSpy SDK in halo.exe; no C existed)
// address 0x619260, size 88 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x619260..0x6192b7: type 1 with the 32-byte challenge.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendClientChallenge(GTI2Connection *connection, const unsigned char *challenge)
{
    int overflow;

    if (!gti2BeginReliableMessage(connection, 0x27, GTI2MsgClientChallenge, &overflow)) {
        return 0;
    }
    if (overflow != 0) {
        return 1;
    }
    gti2BufferWriteData(&connection->outgoingBuffer, challenge, 0x20);
    return gti2SendLastOutgoingMessage(connection) != 0;
}
