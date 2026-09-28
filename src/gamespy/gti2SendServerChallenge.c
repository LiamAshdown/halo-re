// gti2SendServerChallenge  (GameSpy SDK in halo.exe; no C existed)
// address 0x6192c0, size 111 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6192c0..0x61932e: type 2 with the 32-byte response and our 32-byte challenge;
//   the challenge clock starts at the send.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendServerChallenge(GTI2Connection *connection, const unsigned char *response, const unsigned char *challenge)
{
    int overflow;

    if (!gti2BeginReliableMessage(connection, 0x47, GTI2MsgServerChallenge, &overflow)) {
        return 0;
    }
    if (overflow != 0) {
        return 1;
    }
    gti2BufferWriteData(&connection->outgoingBuffer, response, 0x20);
    gti2BufferWriteData(&connection->outgoingBuffer, challenge, 0x20);
    if (!gti2SendLastOutgoingMessage(connection)) {
        return 0;
    }
    connection->challengeTime = connection->lastSend;
    return 1;
}
