// SBReleaseStr  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f2a0, size 76 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f2a0..0x61f2eb: drops a reference; the last one removes the entry (the table
//   frees the string).
// blam-cc: cdecl

#include "gamespy.h"

void SBReleaseStr(void *slist, const char *str)
{
    SBKeyValuePair ref;
    SBKeyValuePair *found;

    ref.key = str;
    found = (SBKeyValuePair *)TableLookup(SBRefStrHash(slist), &ref);
    if (found != 0) {
        found->value = (const char *)((int)found->value - 1);
        if (found->value == 0) {
            TableRemove(SBRefStrHash(slist), &ref);
        }
    }
}
