// gti2HandleClientResponse  (GameSpy SDK in halo.exe; no C existed)
// address 0x618f10, size 291 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618f10..0x619032: ECX len, EDX data, stack connection: while awaiting the
//   client response, at least 0x30 bytes whose first 32 match the expected response: keeps the peer  public key
//   (bytes 32..47) and derives the shared key. Without a connect-attempt callback the peer is told it is closed and
//   the connection closes; otherwise it awaits accept/reject and the callback hears (latency since the challenge, the
//   rest as the message). Anything else is a negotiation error.
// blam-cc: ECX -> len, EDX -> data, stack -> connection

#include "gamespy.h"

#include "gt2.h"

int gti2HandleClientResponse(GTI2Connection *connection, const unsigned char *data, int len)
{
    char hex[0x24];

    if (connection->state != GTI2AwaitingClientResponse || len < 0x30 || !gti2CheckResponse(data, connection->response)) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    memcpy(connection->remotePublicKey, data + 0x20, 0x10);
    gt2_bignum_to_hex(connection->remotePublicKey, hex);
    gt2_bignum_mod_exp(hex, connection->privateExponent, connection->modulus, connection->key);
    if (connection->socket->connectAttemptCallback == 0) {
        if (!gti2ConnectionSendClosed(connection)) {
            return 0;
        }
        gti2ConnectionClosed(connection);
        return 1;
    }
    connection->state = GTI2AwaitingAcceptReject;
    return gti2ConnectAttemptCallback(connection->socket, connection, connection->ip, connection->port,
               (int)(current_time() - connection->challengeTime), data + 0x30, len - 0x30) != 0;
}
