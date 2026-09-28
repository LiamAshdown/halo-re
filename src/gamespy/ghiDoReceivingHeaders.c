// ghiDoReceivingHeaders  (GameSpy SDK in halo.exe; no C existed)
// address 0x621880, size 924 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621880..0x621c1b: receives up to 4 KB of headers until a blank line
//   ("\\r\\n\\r\\n", else "\\n\\n" with the same offsets; closed before it: bad response 7). 1xx: the rest is kept
//   and the status is read again (5). 3xx: a Location becomes the redirect (absolute, or "http://host:port" + path;
//   over 10 redirects: 11; no memory: 1). Otherwise Content-Length and chunked encoding are noted; HEAD-like types
//   (3, 4) and an empty body complete, else the body so far goes to the file data (receiving the file, 7).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiDoReceivingHeaders(GHIConnection *connection)
{
    char buffer[0x1000];
    int len = 0x1000;
    int result = ghiDoReceive(connection, buffer, &len);
    char *headers;
    char *end;
    char *data;
    char *body;
    int bodyLen;
    int oldLen;
    char *contentLength;

    if (result == 3) {
        return;
    }
    if (result == 1) {
        if (connection->recvBuffer.pos == connection->recvBuffer.len) {
            return;
        }
    } else if (result == 0) {
        if (!ghiAppendDataToBuffer(&connection->recvBuffer, buffer, len)) {
            return;
        }
    }
    headers = connection->recvBuffer.data + connection->recvBuffer.pos;
    end = strstr(headers, "\r\n\r\n");
    if (end == 0) {
        end = strstr(headers, "\n\n");
    }
    if (end == 0) {
        if (result == 2) {
    connection->completed = 1;
    connection->result = 7;
            connection->socketError = WSAGetLastError();
        }
        return;
    }
    end += 2;
    *end = 0;
    data = connection->recvBuffer.data;
    oldLen = connection->recvBuffer.len;
    body = end + 2;
    connection->recvBuffer.len = (int)(end - data);
    bodyLen = oldLen - (int)(body - data);
    if (connection->statusCode / 100 == 1) {
        if (bodyLen != 0) {
            memmove(data, body, bodyLen + 1);
            connection->recvBuffer.len = bodyLen;
            connection->recvBuffer.pos = 0;
        } else {
            ghiResetBuffer(&connection->recvBuffer);
        }
        connection->state = 5;
    ghiCallProgressCallback(connection, 0, 0);
        return;
    }
    if (connection->statusCode / 100 == 3) {
        char *location;

        if (connection->redirectCount > 10) {
    connection->completed = 1;
    connection->result = 0xb;
            return;
        }
        location = strstr(headers, "Location:");
        if (location != 0) {
            char *stop;

            location += 9;
            while (isspace(*location)) {
                location++;
            }
            stop = location;
            while (*stop != 0 && !isspace(*stop)) {
                stop++;
            }
            *stop = 0;
            if (*location == '/') {
                connection->redirectURL = (char *)malloc(strlen(connection->serverAddress) + strlen(location) + 14);
                if (connection->redirectURL == 0) {
        connection->completed = 1;
        connection->result = 1;
                }
                sprintf(connection->redirectURL, "http://%s:%d%s", connection->serverAddress, connection->serverPort,
                    location);
                return;
            }
            connection->redirectURL = _strdup(location);
            if (connection->redirectURL == 0) {
        connection->completed = 1;
        connection->result = 1;
            }
            return;
        }
    }
    contentLength = strstr(headers, "Content-Length:");
    if (contentLength != 0) {
        connection->totalSize = atoi(contentLength + 0x10);
    }
    connection->chunked = strstr(headers, "Transfer-Encoding: chunked") != 0;
    if (connection->chunked != 0) {
        connection->chunkHeader[0] = 0;
        connection->chunkHeaderLen = 0;
        connection->chunkBytesLeft = 0;
        connection->chunkReadingState = 0;
    }
    if (connection->type == 3 || connection->type == 4) {
        connection->completed = 1;
        return;
    }
    connection->state = 7;
    if (contentLength != 0 && connection->totalSize == 0) {
        connection->completed = 1;
        return;
    }
    if (bodyLen > 0) {
        ghiProcessIncomingChunkedData(connection, body, bodyLen);
    }
}
