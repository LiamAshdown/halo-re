// SBServerAddKeyValue  (GameSpy SDK in halo.exe; no C existed)
// address 0x6173f0, size 57 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6173f0..0x617428: enters (ref key, ref value) into the server's key table.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void SBServerAddKeyValue(void *server, const char *key, const char *value)
{
    SBKeyValuePair pair;

    pair.key = SBRefStr(0, key);
    pair.value = SBRefStr(0, value);
    TableEnter(FIELD(server, 0x18, HashTable), &pair);
}
