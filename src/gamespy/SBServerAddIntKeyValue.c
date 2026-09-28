// SBServerAddIntKeyValue  (GameSpy SDK in halo.exe; no C existed)
// address 0x617430, size 95 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617430..0x61748e: the value printed with "%d" (0x0065fb30), then as
//   SBServerAddKeyValue.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void SBServerAddIntKeyValue(void *server, const char *key, int value)
{
    char text[0x14];
    SBKeyValuePair pair;

    sprintf(text, "%d", value);
    pair.key = SBRefStr(0, key);
    pair.value = SBRefStr(0, text);
    TableEnter(FIELD(server, 0x18, HashTable), &pair);
}
