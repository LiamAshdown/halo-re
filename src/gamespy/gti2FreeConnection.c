// gti2FreeConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d040, size 134 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d040..0x61d0c5: frees the connection's initial message (+0x38), buffers
//   (+0x44, +0x50), its four arrays (+0x5c, +0x60, +0xa0, +0xa4) and itself.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void gti2FreeConnection(void *connection)
{
    if (FIELD(connection, 0x38, void *) != 0) {
        free(FIELD(connection, 0x38, void *));
    }
    if (FIELD(connection, 0x44, void *) != 0) {
        free(FIELD(connection, 0x44, void *));
    }
    if (FIELD(connection, 0x50, void *) != 0) {
        free(FIELD(connection, 0x50, void *));
    }
    if (FIELD(connection, 0x5c, DArray) != 0) {
        ArrayFree(FIELD(connection, 0x5c, DArray));
    }
    if (FIELD(connection, 0x60, DArray) != 0) {
        ArrayFree(FIELD(connection, 0x60, DArray));
    }
    if (FIELD(connection, 0xa0, DArray) != 0) {
        ArrayFree(FIELD(connection, 0xa0, DArray));
    }
    if (FIELD(connection, 0xa4, DArray) != 0) {
        ArrayFree(FIELD(connection, 0xa4, DArray));
    }
    free(connection);
}
