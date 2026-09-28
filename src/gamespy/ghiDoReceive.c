// ghiDoReceive  (GameSpy SDK in halo.exe; no C existed)
// address 0x621ed0, size 183 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621ed0..0x621f86: recv into the buffer (room for a NUL; when throttled at most
//   the throttle size, and nothing -- 1 -- until the throttle delay passed). 0 with data (NUL-terminated, length
//   out), 1 would block, 2 closed by the peer, 3 error (socket failed 5).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern int ghiThrottleBufferSize;         // 0x00683dd4
extern unsigned long ghiThrottleTimeDelay; // 0x00683dd8

int ghiDoReceive(GHIConnection *connection, char *buffer, int *bufferLen)
{
    int len = *bufferLen - 1;
    int received;

    if (connection->throttle != 0) {
        unsigned long now = current_time();

        if (now < connection->lastThrottleRecv + ghiThrottleTimeDelay) {
            return 1;
        }
        connection->lastThrottleRecv = now;
        if (len >= ghiThrottleBufferSize) {
            len = ghiThrottleBufferSize;
        }
    }
    received = recv(connection->socket, buffer, len, 0);
    if (received == SOCKET_ERROR) {
        int error = WSAGetLastError();

        if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS) {
            return 1;
        }
        connection->socketError = error;
        connection->completed = 1;
        connection->result = 5;
        connection->connectionClosed = 1;
        return 3;
    }
    if (received == 0) {
        connection->connectionClosed = 1;
        return 2;
    }
    buffer[received] = 0;
    *bufferLen = received;
    return 0;
}
