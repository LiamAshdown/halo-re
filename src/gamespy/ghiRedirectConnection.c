// ghiRedirectConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x620af0, size 163 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620af0..0x620b92: restarts the request at the redirect URL: back to host
//   lookup, the old address and path dropped, the socket shut and closed, buffers and status reset, one more redirect
//   counted.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiRedirectConnection(GHIConnection *connection)
{
    connection->state = 0;
    free(connection->URL);
    connection->URL = connection->redirectURL;
    connection->redirectURL = 0;
    free(connection->serverAddress);
    connection->serverAddress = 0;
    connection->serverIP = 0;
    connection->serverPort = 0;
    free(connection->requestPath);
    connection->requestPath = 0;
    shutdown(connection->socket, 2);
    closesocket(connection->socket);
    connection->socket = INVALID_SOCKET;
    ghiResetBuffer(&connection->sendBuffer);
    ghiResetBuffer(&connection->recvBuffer);
    connection->statusMajorVersion = 0;
    connection->statusMinorVersion = 0;
    connection->statusCode = 0;
    connection->statusStringIndex = 0;
    connection->connectionClosed = 0;
    connection->redirectCount++;
}
