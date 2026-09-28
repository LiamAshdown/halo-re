// gt2SetSocketData  (GameSpy SDK in halo.exe; no C existed)
// address 0x614810, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614810..0x61481b: stores the user data at socket +0x30.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void gt2SetSocketData(void *socket, void *data)
{
    FIELD(socket, 0x30, void *) = data;
}
