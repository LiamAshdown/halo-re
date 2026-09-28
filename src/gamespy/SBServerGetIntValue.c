// SBServerGetIntValue  (GameSpy SDK in halo.exe; no C existed)
// address 0x617c10, size 95 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617c10..0x617c6e: "ping" (0x0066b090) is the server's ping (+0x1c); otherwise
//   atoi of the value, or the default when missing or NULL.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int SBServerGetIntValue(void *server, const char *key, int default_value)
{
    SBKeyValuePair pair;
    SBKeyValuePair *found;

    if (strcmp(key, "ping") == 0) {
        return FIELD(server, 0x1c, int);
    }
    pair.key = key;
    found = (SBKeyValuePair *)TableLookup(FIELD(server, 0x18, HashTable), &pair);
    if (found == 0 || found->value == 0) {
        return default_value;
    }
    return atoi(found->value);
}
