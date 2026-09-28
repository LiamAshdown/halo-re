// FreePopularValues  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f3d0, size 135 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f3d0..0x61f456: ESI list: releases every popular value string (the ref-string
//   count, removed at 0) and forgets them.
// blam-cc: ESI -> slist

#include "gamespy.h"

#include "sb.h"

void FreePopularValues(SBServerList *slist)
{
    SBRefString key;
    SBRefString *ref;
    int i;

    for (i = 0; i < slist->numpopularvalues; i++) {
        key.str = slist->popularvalues[i];
        ref = (SBRefString *)TableLookup(SBRefStrHash(slist), &key);
        if (ref != 0 && --ref->refcount == 0) {
            TableRemove(SBRefStrHash(slist), &key);
        }
    }
    slist->numpopularvalues = 0;
}
