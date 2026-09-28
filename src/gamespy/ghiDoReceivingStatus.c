// ghiDoReceivingStatus  (GameSpy SDK in halo.exe; no C existed)
// address 0x621460, size 228 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621460..0x621543: receives up to 1 KB into the receive buffer (closed: bad
//   response 7); once a line is there it is cut, parsed as the status line, and the headers follow (6) from after it.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiDoReceivingStatus(GHIConnection *connection)
{
    char buffer[0x400];
    int len = 0x400;
    int result = ghiDoReceive(connection, buffer, &len);
    char *end;

    if (result == 3) {
        return;
    }
    if (result == 2) {
    connection->completed = 1;
    connection->result = 7;
        connection->socketError = WSAGetLastError();
        return;
    }
    if (result == 0 && !ghiAppendDataToBuffer(&connection->recvBuffer, buffer, len)) {
        return;
    }
    end = strstr(connection->recvBuffer.data, "\r\n");
    if (end == 0) {
        return;
    }
    *end = 0;
    if (ghiParseStatus(connection)) {
        connection->recvBuffer.pos = (int)(end - connection->recvBuffer.data) + 2;
        connection->state = 6;
    ghiCallProgressCallback(connection, 0, 0);
    }
}
