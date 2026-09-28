// gti2HandleClientChallenge  (GameSpy SDK in halo.exe; no C existed)
// address 0x619580, size 162 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x619580..0x619621: ECX connection, stack challenge, len: while awaiting the
//   client challenge with at least 32 bytes (else a negotiation error): answers with the response to it and a new
//   challenge of our own (whose response we keep), then awaits the client response (3).
// blam-cc: ECX -> connection, stack -> challenge, len

#include "gamespy.h"

#include "gt2.h"

int gti2HandleClientChallenge(GTI2Connection *connection, const unsigned char *challenge, int len)
{
    unsigned char our_challenge[0x20];
    unsigned char response[0x20];

    if (connection->state != GTI2AwaitingClientChallenge || len < 0x20) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    gti2GetResponse(response, challenge);
    gti2GetChallenge(our_challenge);
    gti2GetResponse(connection->response, our_challenge);
    if (!gti2SendServerChallenge(connection, response, our_challenge)) {
        return 0;
    }
    connection->state = GTI2AwaitingClientResponse;
    return 1;
}
