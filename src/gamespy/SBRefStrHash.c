// SBRefStrHash  (GameSpy SDK in halo.exe; no C existed)
// address 0x617bc0, size 72 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617bc0..0x617c07: the per-thread ref-string table (a __declspec(thread)
//   variable, TLS +4), created on first use as TableNew2(8, 500, 4, KeyValHashKeyA, KeyValCompareKeyA, SBRefStrFree).
// blam-cc: cdecl

#include "gamespy.h"

__declspec(thread) HashTable g_SBRefStrList;

HashTable SBRefStrHash(void)
{
    if (g_SBRefStrList == 0) {
        g_SBRefStrList = TableNew2(sizeof(SBKeyValuePair), 500, 4, KeyValHashKeyA, KeyValCompareKeyA, SBRefStrFree);
    }
    return g_SBRefStrList;
}
