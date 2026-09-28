// gti2ReceiveMessages  (GameSpy SDK in halo.exe; no C existed)
// address 0x619b90, size 250 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x619b90..0x619c89: while the socket can receive: recvfrom into a 0xffff-byte
//   stack buffer; WSAECONNRESET goes to gti2HandleConnectionReset, WSAEMSGSIZE is ignored, other errors are a socket
//   error (0); data goes to gti2HandleMessage. 0 as soon as a handler fails.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2ReceiveMessages(GTI2Socket *socket)
{
    unsigned char buffer[0x10000];
    struct sockaddr_in address;
    int address_len;
    int len;

    while (CanReceiveOnSocket(socket->socket)) {
        address_len = 0x10;
        len = recvfrom(socket->socket, (char *)buffer, 0xffff, 0, (struct sockaddr *)&address, &address_len);
        if (len == SOCKET_ERROR) {
            int error = WSAGetLastError();

            if (error == WSAECONNRESET) {
                if (!gti2HandleConnectionReset(socket, address.sin_addr.s_addr, ntohs(address.sin_port))) {
                    return 0;
                }
            } else if (error != WSAEMSGSIZE) {
                gti2SocketError(socket);
                return 0;
            }
        } else if (!gti2HandleMessage(socket, buffer, len, address.sin_addr.s_addr, ntohs(address.sin_port))) {
            return 0;
        }
    }
    return 1;
}
