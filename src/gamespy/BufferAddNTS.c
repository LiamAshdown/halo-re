// BufferAddNTS  (GameSpy SDK in halo.exe; no C existed)
// address 0x61eb50, size 59 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61eb50..0x61eb8a: EAX string (NULL: ""), EDX write pointer, EBX length: appends
//   it with its NUL and advances both.
// blam-cc: EAX -> str, EDX -> buffer, EBX -> len

#include "gamespy.h"

#include "sb.h"

void BufferAddNTS(char **buffer, int *len, const char *str)
{
    int n;

    if (str == 0) {
        str = "";
    }
    n = (int)strlen(str) + 1;
    memcpy(*buffer, str, n);
    *len += n;
    *buffer += n;
}
