// qr2_buffer_add  (GameSpy SDK in halo.exe; no C existed)
// address 0x615590, size 103 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x615590..0x6155f6: appends the string with its NUL to a 0x800 byte buffer
//   (length at +0x800), truncated to the room left and kept NUL-terminated.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void qr2_buffer_add(char *buffer, const char *value)
{
    int copy = (int)strlen(value) + 1;
    int room = 0x800 - FIELD(buffer, 0x800, int);

    if (copy > room) {
        copy = room;
    }
    if (copy == 0) {
        return;
    }
    memcpy(buffer + FIELD(buffer, 0x800, int), value, copy);
    FIELD(buffer, 0x800, int) += copy;
    buffer[FIELD(buffer, 0x800, int) - 1] = 0;
}
