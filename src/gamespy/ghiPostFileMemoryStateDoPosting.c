// ghiPostFileMemoryStateDoPosting  (GameSpy SDK in halo.exe; no C existed)
// address 0x6227c0, size 67 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6227c0..0x622802: EDI state, stack connection: sends the rest of the memory
//   file directly; 1 all sent, 2 partial, 0 error.
// blam-cc: EDI -> state, stack -> connection

#include "gamespy.h"

#include "ghttp.h"

int ghiPostFileMemoryStateDoPosting(GHIPostState *state, GHIConnection *connection)
{
    int len = state->data->data.fileMemory.len;
    int remaining;
    int sent;

    if (len == 0) {
        return 1;
    }
    remaining = len - state->pos;
    sent = ghiDoSend(connection, state->data->data.fileMemory.buffer + state->pos, remaining);
    if (sent == -1) {
        return 0;
    }
    state->pos += sent;
    return (sent != remaining) + 1;
}
