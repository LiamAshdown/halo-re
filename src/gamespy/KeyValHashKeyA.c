// KeyValHashKeyA  (GameSpy SDK in halo.exe; no C existed)
// address 0x617ba0, size 20 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617ba0..0x617bb3: the key/value hash: the string hash (0x617a40, EAX string) of
//   the key.
// blam-cc: cdecl

#include "gamespy.h"
#include <ctype.h>

// 0x617a40 (EAX string, stack buckets): the case-insensitive string hash (hash * -1664117991 + tolower(c)),
//   unsigned modulo the bucket count. Only KeyValHashKeyA calls it; a static here.
static int string_hash(const char *s, int num_buckets)
{
    unsigned int hash = 0;

    for (; *s != 0; s++) {
        hash = (unsigned int)tolower(*s) - hash * 0x63306ce7;
    }
    return (int)(hash % (unsigned int)num_buckets);
}

int KeyValHashKeyA(const void *elem, int num_buckets)
{
    return string_hash(((const SBKeyValuePair *)elem)->key, num_buckets);
}
