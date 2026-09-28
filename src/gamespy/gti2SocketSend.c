// gti2SocketSend  (GameSpy SDK in halo.exe; no C existed)
// address 0x61cbe0, size 304 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61cbe0..0x61cd0f: (socket, ip, port, message, len, arg6, arg7): 1 (nothing
//   sent) when select says the socket cannot send; a failed sendto of WSAECONNRESET goes to gti2HandleConnectionReset
//   (0 when that fails), WSAEMSGSIZE is ignored, anything else is a socket error (0). After a good send the send dump
//   callback (when set) hears (socket, its connection, ip, port, 0, message, len, 1, arg6, arg7); 0 when that freed
//   the socket.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SocketSend(GTI2Socket *socket, unsigned int ip, unsigned short port, const unsigned char *message, int len,
    int arg6, int arg7)
{
    struct sockaddr_in address;

    gti2MessageCheck((const char **)&message, &len);
    if (!CanSendOnSocket(socket->socket)) {
        return 1;
    }
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = ip;
    address.sin_port = htons(port);
    if (sendto(socket->socket, (const char *)message, len, 0, (const struct sockaddr *)&address, 0x10) == SOCKET_ERROR) {
        int error = WSAGetLastError();

        if (error == WSAECONNRESET) {
            if (!gti2HandleConnectionReset(socket, ip, port)) {
                return 0;
            }
        } else if (error != WSAEMSGSIZE) {
            gti2SocketError(socket);
            return 0;
        }
    } else if (socket->sendDumpCallback != 0) {
        if (!gti2DumpCallback(socket, gti2SocketFindConnection(socket, ip, port), ip, port, 0, message, len, 1, arg6,
                arg7)) {
            return 0;
        }
    }
    return 1;
}
