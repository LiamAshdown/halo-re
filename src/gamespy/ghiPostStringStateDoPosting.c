// ghiPostStringStateDoPosting  (GameSpy SDK in halo.exe; no C existed)
// address 0x6225f0, size 261 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6225f0..0x6226f4: EAX state, EDX connection: an empty value is done. Url-
//   encoded with invalid characters: the whole string is escaped into the send buffer (safe characters as is, space
//   "+", others "%XX" from the SIGNED character -- a high one indexes before the digit table in the binary).
//   Otherwise sent directly: from the START of the string each time (the position is only counted); 1 done, 2
//   partial, 0 error.
// blam-cc: EAX -> state, EDX -> connection

#include "gamespy.h"

#include "ghttp.h"

int ghiPostStringStateDoPosting(GHIPostState *state, GHIConnection *connection)
{
    static const char digits[] = "0123456789ABCDEF";
    GHIPostData *field = state->data;
    int len = field->data.string.len;
    int remaining;
    int sent;

    if (len == 0) {
        return 1;
    }
    if (connection->post->useMultipart == 0 && field->data.string.invalidChars != 0) {
        const char *c = field->data.string.string;
        char hex[4] = {'%', '0', '0', 0};

        for (; *c != 0; c++) {
            int value = *c;

            if (strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_@-.*", value) != 0) {
                ghiAppendCharToBuffer(&connection->sendBuffer, value);
            } else if (value == ' ') {
                ghiAppendCharToBuffer(&connection->sendBuffer, '+');
            } else {
                hex[1] = digits[value / 16];
                hex[2] = digits[value % 16];
                ghiAppendDataToBuffer(&connection->sendBuffer, hex, 3);
            }
        }
        return 1;
    }
    remaining = len - state->pos;
    sent = ghiDoSend(connection, field->data.string.string, remaining);
    if (sent == -1) {
        return 0;
    }
    state->pos += sent;
    return (sent != remaining) + 1;
}
