// gti2HandleServerChallenge  (GameSpy SDK in halo.exe; no C existed)
// address 0x619630, size 183 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x619630..0x6196e6: ECX data, EDX connection, stack len: while awaiting the
//   server challenge, at least 64 bytes whose first 32 are the expected response (else a negotiation error): the
//   client response (response to the server  challenge, our public key, the initial message) goes out, the initial
//   message is freed and it awaits acceptance (1).
// blam-cc: ECX -> data, EDX -> connection, stack -> len

#include "gamespy.h"

#include "gt2.h"

int gti2HandleServerChallenge(GTI2Connection *connection, const unsigned char *data, int len)
{
    unsigned char response[0x20];

    if (connection->state != GTI2AwaitingServerChallenge || len < 0x40 || !gti2CheckResponse(data, connection->response)) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    gti2GetResponse(response, data + 0x20);
    if (!gti2SendClientResponse(connection, response, connection->initialMessage, connection->initialMessageLen)) {
        return 0;
    }
    if (connection->initialMessage != 0) {
        free(connection->initialMessage);
        connection->initialMessage = 0;
    }
    connection->state = GTI2AwaitingAcceptance;
    return 1;
}
