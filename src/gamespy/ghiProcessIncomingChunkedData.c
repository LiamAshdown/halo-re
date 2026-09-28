// ghiProcessIncomingChunkedData  (GameSpy SDK in halo.exe; no C existed)
// address 0x621680, size 503 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621680..0x621876: EAX data, stack connection, length: without chunked encoding
//   straight to the file data; otherwise the chunk state machine: header line (hex size; bad: bad response 7; 0 means
//   done), data, trailing line, done (completed).
// blam-cc: EAX -> buffer, stack -> connection, len

#include "gamespy.h"

#include "ghttp.h"

int ghiProcessIncomingChunkedData(GHIConnection *connection, char *buffer, int len)
{
    if (connection->chunked == 0) {
        return ghiProcessIncomingFileData(connection, buffer, len);
    }
    while (len > 0) {
        if (connection->chunkReadingState == 0) {
            char *newline = strchr(buffer, '\n');
            int n;
            int size;

            if (newline == 0) {
                ghiChunkHeaderAppend(connection, buffer, len);
                return 1;
            }
            n = (int)(newline - buffer) - 1;
            if (n != 0 && connection->chunkHeaderLen < 10) {
                if (10 - connection->chunkHeaderLen < n) {
                    n = 10 - connection->chunkHeaderLen;
                }
                memcpy(connection->chunkHeader + connection->chunkHeaderLen, buffer, n);
                connection->chunkHeaderLen += n;
                connection->chunkHeader[connection->chunkHeaderLen] = 0;
            }
            len -= (int)(newline + 1 - buffer);
            buffer = newline + 1;
            if (sscanf(connection->chunkHeader, "%x", &size) != 1) {
                size = -1;
            }
            connection->chunkBytesLeft = size;
            if (size == -1) {
    connection->completed = 1;
    connection->result = 7;
                return 0;
            }
            connection->chunkReadingState = size == 0 ? 3 : 1;
        } else if (connection->chunkReadingState == 1) {
            int n = len;

            if (connection->chunkBytesLeft < n) {
                n = connection->chunkBytesLeft;
            }
            if (!ghiProcessIncomingFileData(connection, buffer, n)) {
                return 0;
            }
            len -= n;
            buffer += n;
            connection->chunkBytesLeft -= n;
            if (connection->chunkBytesLeft == 0) {
                connection->chunkReadingState = 2;
            }
        } else if (connection->chunkReadingState == 2) {
            char *newline = strchr(buffer, '\n');

            if (newline == 0) {
                return 1;
            }
            len -= (int)(newline + 1 - buffer);
            buffer = newline + 1;
            connection->chunkHeader[0] = 0;
            connection->chunkHeaderLen = 0;
            connection->chunkBytesLeft = 0;
            connection->chunkReadingState = 0;
        } else {
            if (connection->chunkReadingState != 3) {
                return 0;
            }
            connection->completed = 1;
            return 1;
        }
    }
    return 1;
}
