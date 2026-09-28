// ghiDoReceivingFile  (GameSpy SDK in halo.exe; no C existed)
// address 0x621c20, size 166 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621c20..0x621cc5: until complete: receive up to 8 KB and hand it to the
//   (chunked) file data; nothing waiting or an error stops, the peer closing completes.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiDoReceivingFile(GHIConnection *connection)
{
    char buffer[0x2000];

    if (connection->completed != 0) {
        return;
    }
    for (;;) {
        int len = 0x2000;
        int result = ghiDoReceive(connection, buffer, &len);

        if (result == 3 || result == 1) {
            return;
        }
        if (result == 2) {
            connection->completed = 1;
            return;
        }
        if (!ghiProcessIncomingChunkedData(connection, buffer, len)) {
            return;
        }
        if (connection->completed != 0) {
            return;
        }
    }
}
