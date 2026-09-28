// ghiPostFileDiskStateDoPosting  (GameSpy SDK in halo.exe; no C existed)
// address 0x622700, size 183 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622700..0x6227b6: ESI state, EDI connection: 4 KB reads of the file (a short or
//   over-long read fails with file read failed 14) sent or queued until the whole file went (1); a queued part
//   returns 2.
// blam-cc: ESI -> state, EDI -> connection

#include "gamespy.h"

#include "ghttp.h"

int ghiPostFileDiskStateDoPosting(GHIPostState *state, GHIConnection *connection)
{
    char buffer[0x1000];

    for (;;) {
        int n = (int)fread(buffer, 1, 0x1000, state->file);
        int result;

        if (n <= 0) {
            break;
        }
        state->pos += n;
        if (state->pos > state->len) {
            break;
        }
        result = ghiTrySendThenBuffer(connection, buffer, n);
        if (result == 0) {
            return 0;
        }
        if (state->pos == state->len) {
            return 1;
        }
        if (result != 1) {
            return 2;
        }
    }
    connection->completed = 1;
    connection->result = 0xe;
    return 0;
}
