// goastrdup  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d270, size 61 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d270..0x61d2ac: a malloc copy of the string; NULL for NULL (or when malloc
//   fails).
// blam-cc: cdecl

#include "gamespy.h"

char *goastrdup(const char *src)
{
    char *copy;

    if (src == 0) {
        return 0;
    }
    copy = (char *)malloc(strlen(src) + 1);
    if (copy != 0) {
        strcpy(copy, src);
    }
    return copy;
}
