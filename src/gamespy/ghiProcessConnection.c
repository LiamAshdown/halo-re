// ghiProcessConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x61bc00, size 243 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61bc00..0x61bcf2: unless already inside, runs every state step in order
//   (lookup, connect, send, post, wait, status, headers, file), follows a pending redirect; once complete: the status
//   sets the result, the save file closes, the completed callback runs and the connection is freed (the completed
//   flag returned); else 0.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiProcessConnection(GHIConnection *connection)
{
    int completed;

    if (connection->processing != 0) {
        return 0;
    }
    connection->processing = 1;
    if (connection->state == 0) {
        ghiDoHostLookup(connection);
    }
    if (connection->state == 1) {
        ghiDoConnecting(connection);
    }
    if (connection->state == 2) {
        ghiDoSendingRequest(connection);
    }
    if (connection->state == 3) {
        ghiDoPosting(connection);
    }
    if (connection->state == 4) {
        ghiDoWaiting(connection);
    }
    if (connection->state == 5) {
        ghiDoReceivingStatus(connection);
    }
    if (connection->state == 6) {
        ghiDoReceivingHeaders(connection);
    }
    if (connection->state == 7) {
        ghiDoReceivingFile(connection);
    }
    if (connection->redirectURL != 0) {
        ghiRedirectConnection(connection);
    }
    completed = connection->completed;
    if (completed == 0) {
        connection->processing = 0;
        return 0;
    }
    ghiHandleStatus(connection);
    if (connection->saveFile != 0) {
        fclose(connection->saveFile);
        connection->saveFile = 0;
    }
    ghiCallCompletedCallback(connection);
    ghiFreeConnection(connection);
    return completed;
}
