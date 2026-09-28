// FreeKeyList  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f460, size 147 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f460..0x61f4f2: ESI list: releases every key name string and frees the key
//   list.
// blam-cc: ESI -> slist

#include "gamespy.h"

#include "sb.h"

void FreeKeyList(SBServerList *slist)
{
    SBRefString key;
    SBRefString *ref;
    int i;

    if (slist->keylist == 0) {
        return;
    }
    for (i = 0; i < ArrayLength(slist->keylist); i++) {
        key.str = ((SBKeyInfo *)ArrayNth(slist->keylist, i))->name;
        ref = (SBRefString *)TableLookup(SBRefStrHash(slist), &key);
        if (ref != 0 && --ref->refcount == 0) {
            TableRemove(SBRefStrHash(slist), &key);
        }
    }
    ArrayFree(slist->keylist);
    slist->keylist = 0;
}
