// ghiDoConnecting  (GameSpy SDK in halo.exe; no C existed)
// address 0x620fa0, size 371 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620fa0..0x621112: first time: a non-blocking TCP socket (socket failed 5), a
//   throttled receive buffer, connect to the proxy or server port (anything but would-block / in-progress: connect
//   failed 6). Then select: writable means sending (2); an exception or select failure fails with 6.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern char *ghiProxyAddress;             // 0x007231e0
extern unsigned short ghiProxyPort;       // 0x007231dc
extern int ghiThrottleBufferSize;         // 0x00683dd4
extern unsigned long ghiThrottleTimeDelay; // 0x00683dd8

extern int SetSockBlocking(SOCKET sock, int is_blocking);
extern int SetReceiveBufferSize(SOCKET sock, int size);

void ghiDoConnecting(GHIConnection *connection)
{
    int writeFlag;
    int exceptFlag;
    int result;

    if (connection->socket == INVALID_SOCKET) {
        struct sockaddr_in address;

        connection->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connection->socket == INVALID_SOCKET || !SetSockBlocking(connection->socket, 0)) {
    connection->completed = 1;
    connection->result = 5;
            connection->socketError = WSAGetLastError();
            return;
        }
        if (connection->throttle != 0) {
            SetReceiveBufferSize(connection->socket, ghiThrottleBufferSize);
        }
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(ghiProxyAddress != 0 ? ghiProxyPort : connection->serverPort);
        address.sin_addr.s_addr = connection->serverIP;
        if (connect(connection->socket, (const struct sockaddr *)&address, 0x10) == SOCKET_ERROR) {
            int error = WSAGetLastError();

            if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS) {
    connection->completed = 1;
    connection->result = 6;
                connection->socketError = error;
                return;
            }
        }
    }
    result = ghiSocketSelect(connection->socket, 0, &writeFlag, &exceptFlag);
    if (result != 0 && exceptFlag == 0) {
        if (writeFlag == 0) {
            return;
        }
        connection->state = 2;
    ghiCallProgressCallback(connection, 0, 0);
        return;
    }
    connection->completed = 1;
    connection->result = 6;
    if (result == 0) {
        connection->socketError = WSAGetLastError();
    }
}
