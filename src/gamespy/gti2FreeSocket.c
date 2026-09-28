// gti2FreeSocket  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c620, size 62 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c620..0x61c65d: inside a callback only marks the socket to close (+0x14);
//   otherwise closes it, frees its connection table, closed connections and itself, and WSACleanup.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void gti2FreeSocket(void *socket)
{
    if (FIELD(socket, 0x18, int) != 0) {
        FIELD(socket, 0x14, int) = 1;
        return;
    }
    closesocket(FIELD(socket, 0x00, SOCKET));
    TableFree(FIELD(socket, 0x0c, HashTable));
    ArrayFree(FIELD(socket, 0x10, DArray));
    free(socket);
    WSACleanup();
}
