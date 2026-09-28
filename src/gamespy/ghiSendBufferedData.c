// ghiSendBufferedData  (GameSpy SDK in halo.exe; no C existed)
// address 0x622e10, size 134 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622e10..0x622e95: sends the connection  send buffer (the buffer argument is not
//   used) while the socket is writable; select failure or a socket exception fails with socket failed (5). 1 once all
//   is sent or the socket is busy, 0 on a send error.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiSendBufferedData(GHIBuffer *buffer, GHIConnection *connection)
{
    (void)buffer;
    for (;;) {
        int writeFlag;
        int exceptFlag;
        int sent;

        if (!ghiSocketSelect(connection->socket, 0, &writeFlag, &exceptFlag) || exceptFlag != 0) {
    connection->completed = 1;
    connection->result = 5;
            connection->socketError = WSAGetLastError();
            return 0;
        }
        if (writeFlag == 0) {
            return 1;
        }
        sent = ghiDoSend(connection, connection->sendBuffer.data + connection->sendBuffer.pos,
            connection->sendBuffer.len - connection->sendBuffer.pos);
        if (sent == -1) {
            return 0;
        }
        connection->sendBuffer.pos += sent;
        if (connection->sendBuffer.pos >= connection->sendBuffer.len) {
            return 1;
        }
    }
}
