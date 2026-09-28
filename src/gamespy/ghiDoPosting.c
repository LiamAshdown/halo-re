// ghiDoPosting  (GameSpy SDK in halo.exe; no C existed)
// address 0x6212d0, size 93 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6212d0..0x62132c: posts what it can: a failure cleans the posting state;
//   progress in bytes reaches the post callback; when done the state is cleaned and it waits (4).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiDoPosting(GHIConnection *connection)
{
    int bytesPosted = connection->bytesPosted;
    int result = ghiPostDoPosting(connection);

    if (result == 0) {
        ghiPostCleanupState(connection);
        return;
    }
    if (bytesPosted != connection->bytesPosted) {
        ghiCallPostCallback(connection);
    }
    if (result == 1) {
        ghiPostCleanupState(connection);
        connection->state = 4;
    ghiCallProgressCallback(connection, 0, 0);
    }
}
