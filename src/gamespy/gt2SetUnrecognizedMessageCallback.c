// gt2SetUnrecognizedMessageCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x614800, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614800..0x61480b: stores the callback at socket +0x2c.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void gt2SetUnrecognizedMessageCallback(void *socket, void *callback)
{
    FIELD(socket, 0x2c, void *) = callback;
}
