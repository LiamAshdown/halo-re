// ghiProcessIncomingFileData  (GameSpy SDK in halo.exe; no C existed)
// address 0x621550, size 208 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621550..0x62161f: EAX length, EBX data, ESI connection: counts the bytes (all
//   of the total, or a closed connection, completes it); a get appends to the file buffer, a save writes the file
//   (short write: file write failed 13), a stream passes it through; progress. Other types report progress with the
//   binary  stale ECX (here NULL, 0).
// blam-cc: EAX -> len, EBX -> buffer, ESI -> connection

#include "gamespy.h"

#include "ghttp.h"

int ghiProcessIncomingFileData(GHIConnection *connection, char *buffer, int len)
{
    connection->fileBytesReceived += len;
    if (connection->fileBytesReceived == connection->totalSize || connection->connectionClosed != 0) {
        connection->completed = 1;
    }
    if (connection->type == 0) {
        if (!ghiAppendDataToBuffer(&connection->getFileBuffer, buffer, len)) {
            return 0;
        }
        ghiCallProgressCallback(connection, connection->getFileBuffer.data, connection->getFileBuffer.len);
        return 1;
    }
    if (connection->type == 1) {
        if ((int)fwrite(buffer, 1, len, connection->saveFile) != len) {
    connection->completed = 1;
    connection->result = 0xd;
            return 0;
        }
        ghiCallProgressCallback(connection, buffer, len);
        return 1;
    }
    if (connection->type == 2) {
        ghiCallProgressCallback(connection, buffer, len);
        return 1;
    }
    ghiCallProgressCallback(connection, 0, 0);
    return 1;
}
