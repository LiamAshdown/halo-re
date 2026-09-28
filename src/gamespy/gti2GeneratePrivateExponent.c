// gti2GeneratePrivateExponent  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c670, size 149 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c670..0x61c704: EDI buffer: 16 upper-case hex digits (|rand()| % 16) and a
//   NUL.
// blam-cc: EDI -> hex

#include "gamespy.h"

#include "gt2.h"

void gti2GeneratePrivateExponent(char *hex)
{
    static const char digits[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    int i;

    for (i = 0; i < 0x10; i++) {
        hex[i] = digits[abs(rand()) % 16];
    }
    hex[i] = 0;
}
