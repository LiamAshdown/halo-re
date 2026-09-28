// KeyValCompareKeyA  (GameSpy SDK in halo.exe; no C existed)
// address 0x617340, size 26 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617340..0x617359: the key/value comparator: _stricmp of the two keys
//   (0x628d8b).
// blam-cc: cdecl

#include "gamespy.h"

int KeyValCompareKeyA(const void *elem1, const void *elem2)
{
    return _stricmp(((const SBKeyValuePair *)elem1)->key, ((const SBKeyValuePair *)elem2)->key);
}
