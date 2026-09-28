// gt2SetConnectionData  (GameSpy SDK in halo.exe; no C existed)
// address 0x614830, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614830..0x61483b: stores the user data at connection +0x40.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void gt2SetConnectionData(void *connection, void *data)
{
    FIELD(connection, 0x40, void *) = data;
}
