// ghiTrySendThenBuffer  (GameSpy SDK in halo.exe; no C existed)
// address 0x621ff0, size 88 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621ff0..0x622047: sends directly when nothing is queued (all sent: 1; error:
//   0); the rest is queued in the send buffer (2, or 0 when that fails).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiTrySendThenBuffer(GHIConnection *connection, const char *buffer, int len)
{
    int sent = 0;

    if (connection->sendBuffer.len == 0) {
        sent = ghiDoSend(connection, buffer, len);
        if (sent == -1) {
            return 0;
        }
        if (sent == len) {
            return 1;
        }
    }
    return ghiAppendDataToBuffer(&connection->sendBuffer, buffer + sent, len - sent) ? 2 : 0;
}
