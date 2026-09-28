// SBServerKeyValFree  (GameSpy SDK in halo.exe; no C existed)
// address 0x617a80, size 31 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617a80..0x617a9e: a server key table element free: releases the key and the
//   value ref strings.
// blam-cc: cdecl

#include "gamespy.h"

void SBServerKeyValFree(void *elem)
{
    SBKeyValuePair *pair = (SBKeyValuePair *)elem;

    SBReleaseStr(0, pair->key);
    SBReleaseStr(0, pair->value);
}
