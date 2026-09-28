// SBServerGetBoolValue  (GameSpy SDK in halo.exe; no C existed)
// address 0x6174d0, size 86 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6174d0..0x617525: the key as a bool: the default when missing or NULL, 0 for a
//   value starting with 0 F f N n, else 1.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int SBServerGetBoolValue(void *server, const char *key, int default_value)
{
    SBKeyValuePair pair;
    SBKeyValuePair *found;
    char first;

    pair.key = key;
    found = (SBKeyValuePair *)TableLookup(FIELD(server, 0x18, HashTable), &pair);
    if (found == 0 || found->value == 0) {
        return default_value;
    }
    first = found->value[0];
    if (first == '0' || first == 'F' || first == 'f' || first == 'N' || first == 'n') {
        return 0;
    }
    return 1;
}
