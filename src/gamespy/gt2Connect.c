// gt2Connect  (GameSpy SDK in halo.exe; no C existed)
// address 0x6145a0, size 265 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6145a0..0x6146a8: (socket, connection out, remote address string, message, len,
//   timeout, callbacks, blocking): 4 for an unparsable address or a zero ip or port; otherwise a new outgoing
//   connection with the timeout starts its attempt (either failure code is returned). Non-blocking hands out the
//   connection (when asked) and returns 0; blocking holds a callback level on the connection and thinks the socket
//   (gt2Think inline) with msleep(1) until the state reaches connected or beyond, hands out the connection only if it
//   is connected, and returns its connect result.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gt2Connect(GTI2Socket *socket, GTI2Connection **connection_out, const char *remote_address, const unsigned char *message,
    int len, unsigned long timeout, const GT2ConnectionCallbacks *callbacks, int blocking)
{
    GTI2Connection *connection;
    unsigned int ip;
    unsigned short port;
    int result;

    if (!gt2StringToAddress(remote_address, &ip, &port) || ip == 0 || port == 0) {
        return 4;
    }
    result = gti2NewOutgoingConnection(socket, &connection, ip, port);
    if (result != 0) {
        return result;
    }
    connection->timeout = timeout;
    result = gti2StartConnectionAttempt(connection, message, len, callbacks);
    if (result != 0) {
        return result;
    }
    if (!blocking) {
        if (connection_out != 0) {
            *connection_out = connection;
        }
        return 0;
    }
    connection->callbackLevel++;
    for (;;) {
        if (gti2ReceiveMessages(socket) && gti2SocketConnectionsThink(socket)) {
            gti2FreeClosedConnections(socket);
        }
        if (connection->state >= GTI2Connected) {
            break;
        }
        msleep(1);
    }
    connection->callbackLevel--;
    if (connection->state == GTI2Connected) {
        *connection_out = connection;
    }
    return connection->connectionResult;
}
