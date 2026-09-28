// ghiDoSend  (GameSpy SDK in halo.exe; no C existed)
// address 0x621f90, size 86 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621f90..0x621fe5: send; would-block is 0 bytes, an error fails with socket
//   failed (-1). While posting the bytes count as posted.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiDoSend(GHIConnection *connection, const char *buffer, int len)
{
    int sent = send(connection->socket, buffer, len, 0);

    if (sent == SOCKET_ERROR) {
        int error = WSAGetLastError();

        if (error == WSAEWOULDBLOCK) {
            return 0;
        }
        connection->socketError = error;
        connection->completed = 1;
        connection->result = 5;
        return -1;
    }
    if (connection->state == 3) {
        connection->bytesPosted += sent;
    }
    return sent;
}
