// gt2GetConnectionData  (GameSpy SDK in halo.exe; no C existed)
// address 0x614840, size 8 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614840..0x614847: the user data at connection +0x40.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void *gt2GetConnectionData(void *connection)
{
    return FIELD(connection, 0x40, void *);
}
