// SBServerGetStringValue  (GameSpy SDK in halo.exe; no C existed)
// address 0x617490, size 49 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617490..0x6174c0: the value of key in the server's key table (+0x18), or the
//   default.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

const char *SBServerGetStringValue(void *server, const char *key, const char *default_value)
{
    SBKeyValuePair pair;
    SBKeyValuePair *found;

    pair.key = key;
    found = (SBKeyValuePair *)TableLookup(FIELD(server, 0x18, HashTable), &pair);
    return found != 0 ? found->value : default_value;
}
