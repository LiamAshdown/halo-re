// ghiParseStatus  (GameSpy SDK in halo.exe; no C existed)
// address 0x621390, size 193 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621390..0x621450: ESI connection: "HTTP/%d.%d %d%n" from the receive buffer,
//   the reason text start after spaces; a major version of at least 1 and a code in 100..599 is kept (1), anything
//   else is a bad response (7).
// blam-cc: ESI -> connection

#include "gamespy.h"

#include "ghttp.h"

int ghiParseStatus(GHIConnection *connection)
{
    int major;
    int minor;
    int code;
    int index;
    int count = sscanf(connection->recvBuffer.data, "HTTP/%d.%d %d%n", &major, &minor, &code, &index);

    while (connection->recvBuffer.data[index] != 0 && isspace(connection->recvBuffer.data[index])) {
        index++;
    }
    if (count == 3 && major >= 1 && code >= 100 && code < 600) {
        connection->statusMajorVersion = major;
        connection->statusCode = code;
        connection->statusMinorVersion = minor;
        connection->statusStringIndex = index;
        return 1;
    }
    connection->completed = 1;
    connection->result = 7;
    return 0;
}
