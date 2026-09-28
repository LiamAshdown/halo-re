// ghiPostDoPosting  (GameSpy SDK in halo.exe; no C existed)
// address 0x6229c0, size 235 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6229c0..0x622aaa: flushes queued data first (partial: 2); posts each remaining
//   field in order (first flag for field 0); multipart ends with the closing boundary. 1 when nothing is left queued,
//   2 while some is, 0 on error.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiPostDoPosting(GHIConnection *connection)
{
    int count = ArrayLength(connection->postingStates);

    if (connection->sendBuffer.len != 0) {
        if (!ghiSendBufferedData(&connection->sendBuffer, connection)) {
            return 0;
        }
        if (connection->sendBuffer.pos < connection->sendBuffer.len) {
            return 2;
        }
        ghiResetBuffer(&connection->sendBuffer);
        if (connection->objectsPosted == count) {
            return 1;
        }
    }
    for (; connection->objectsPosted < count; connection->objectsPosted++) {
        int result = ghiPostStateDoPosting((GHIPostState *)ArrayNth(connection->postingStates, connection->objectsPosted),
            connection, connection->objectsPosted == 0);

        if (result == 0) {
            return 0;
        }
        if (result == 2) {
            return 2;
        }
    }
    if (connection->post->useMultipart != 0 && !ghiTrySendThenBuffer(connection, "\r\n--Qr4G823s23d---<<><><<<>--7d118e0536--\r\n", 0x2b)) {
        return 0;
    }
    return (connection->sendBuffer.len != 0) + 1;
}
