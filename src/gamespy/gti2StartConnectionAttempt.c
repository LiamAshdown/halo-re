// gti2StartConnectionAttempt  (GameSpy SDK in halo.exe; no C existed)
// address 0x61cd90, size 230 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61cd90..0x61ce75: keeps a copy of a non-empty initial message (1 when out of
//   memory) and the callbacks, makes a challenge, keeps the response it expects and sends the client challenge: 0
//   (state 0) or 3.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2StartConnectionAttempt(GTI2Connection *connection, const unsigned char *message, int len,
    const GT2ConnectionCallbacks *callbacks)
{
    unsigned char challenge[0x20];

    gti2MessageCheck((const char **)&message, &len);
    if (len > 0) {
        connection->initialMessage = (unsigned char *)malloc(len);
        if (connection->initialMessage == 0) {
            return 1;
        }
        memcpy(connection->initialMessage, message, len);
        connection->initialMessageLen = len;
    }
    if (callbacks != 0) {
        connection->callbacks = *callbacks;
    }
    gti2GetChallenge(challenge);
    gti2GetResponse(connection->response, challenge);
    if (gti2SendClientChallenge(connection, challenge)) {
        connection->state = GTI2AwaitingServerChallenge;
        return 0;
    }
    return 3;
}
