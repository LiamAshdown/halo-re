// ghiDoSendingRequest  (GameSpy SDK in halo.exe; no C existed)
// address 0x621120, size 424 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621120..0x6212c7: builds the request once (POST / HEAD / GET, the proxy gets
//   the full URL, HTTP/1.1, Host (with the port unless 80), User-Agent GameSpyHTTP/1.0, Connection close, for a post
//   Content-Length and Content-Type, the caller  headers, blank line) and sends what it can; when all went out:
//   posting (3) or waiting (4).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern char *ghiProxyAddress;             // 0x007231e0
extern unsigned short ghiProxyPort;       // 0x007231dc

void ghiDoSendingRequest(GHIConnection *connection)
{
    GHIBuffer *buffer = &connection->sendBuffer;

    if (buffer->len == 0) {
        const char *method;

        if (connection->post != 0) {
            method = "POST ";
        } else if (connection->type == 3) {
            method = "HEAD ";
        } else {
            method = "GET ";
        }
        ghiAppendDataToBuffer(buffer, method, 0);
        ghiAppendDataToBuffer(buffer, ghiProxyAddress != 0 ? connection->URL : connection->requestPath, 0);
        ghiAppendDataToBuffer(buffer, " HTTP/1.1\r\n", 0);
        if (connection->serverPort == 80) {
            ghiAppendHeaderToBuffer(buffer, "Host", connection->serverAddress);
        } else {
            ghiAppendDataToBuffer(buffer, "Host: ", 0);
            ghiAppendDataToBuffer(buffer, connection->serverAddress, 0);
            ghiAppendCharToBuffer(buffer, ':');
            ghiAppendIntToBuffer(buffer, connection->serverPort);
            ghiAppendDataToBuffer(buffer, "\r\n", 2);
        }
        ghiAppendHeaderToBuffer(buffer, "User-Agent", "GameSpyHTTP/1.0");
        ghiAppendHeaderToBuffer(buffer, "Connection", "close");
        if (connection->post != 0) {
            char length[0x10];

            sprintf(length, "%d", connection->totalBytes);
            ghiAppendHeaderToBuffer(buffer, "Content-Length", length);
            ghiAppendHeaderToBuffer(buffer, "Content-Type", ghiPostGetContentType(connection));
        }
        if (connection->sendHeaders != 0) {
            ghiAppendDataToBuffer(buffer, connection->sendHeaders, 0);
        }
        ghiAppendDataToBuffer(buffer, "\r\n", 2);
    }
    if (ghiSendBufferedData(buffer, connection) && buffer->pos >= buffer->len) {
        ghiResetBuffer(buffer);
        connection->state = connection->post != 0 ? 3 : 4;
    ghiCallProgressCallback(connection, 0, 0);
    }
}
