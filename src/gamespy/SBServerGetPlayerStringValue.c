// SBServerGetPlayerStringValue  (GameSpy SDK in halo.exe; no C existed)
// address 0x617530, size 133 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617530..0x6175b4: the value of "<key>_<index>" (0x0064e488 "%s_%d"), or the
//   default.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

const char *SBServerGetPlayerStringValue(void *server, int index, const char *key, const char *default_value)
{
    char name[0x80];
    SBKeyValuePair pair;
    SBKeyValuePair *found;

    sprintf(name, "%s_%d", key, index);
    pair.key = name;
    found = (SBKeyValuePair *)TableLookup(FIELD(server, 0x18, HashTable), &pair);
    return found != 0 ? found->value : default_value;
}
