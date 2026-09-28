// SBRefStr  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f220, size 113 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f220..0x61f290: a reference-counted copy of str from the thread's ref-string
//   table (entries {string, count}): an existing entry gains a reference, otherwise goastrdup starts one at 1. The
//   server list argument is only passed on to SBRefStrHash.
// blam-cc: cdecl

#include "gamespy.h"

const char *SBRefStr(void *slist, const char *str)
{
    SBKeyValuePair ref;
    SBKeyValuePair *found;

    ref.key = str;
    found = (SBKeyValuePair *)TableLookup(SBRefStrHash(slist), &ref);
    if (found != 0) {
        found->value = (const char *)((int)found->value + 1);
        return found->key;
    }
    ref.key = goastrdup(str);
    ref.value = (const char *)1;
    TableEnter(SBRefStrHash(slist), &ref);
    return ref.key;
}
