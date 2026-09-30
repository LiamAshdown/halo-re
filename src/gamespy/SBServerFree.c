// SBServerFree  (GameSpy SDK in halo.exe; no C existed)
// address 0x6173c0, size 34 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6173c0..0x6173e1: takes a pointer to the server: frees its key table (clearing
//   the field) and the server.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void SBServerFree(void *elem)
{
    void *server = *(void **)elem;

    TableFree(FIELD(server, 0x18, HashTable));
    FIELD(server, 0x18, HashTable) = 0;
    free(server);
}
