// gt2_bignum_to_hex  (GameSpy SDK in halo.exe; no C existed)
// address 0x618150, size 109 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618150..0x6181bc: upper-case hex of the number without its leading zero bytes
//   (the skip is unbounded; an all-zero number gives ""), NUL terminated.
// blam-cc: cdecl

#include "gamespy.h"

extern unsigned int gt2_bignum_length;          // 0x00683944, bytes per big number
extern unsigned char gt2_bignum_modulus[0x400]; // 0x00723240

void gt2_bignum_to_hex(const unsigned char *number, char *hex)
{
    unsigned int i = 0;

    while (number[i] == 0) {
        i++;
    }
    for (; (int)i < (int)gt2_bignum_length; i++) {
        unsigned char high = (unsigned char)(number[i] >> 4);
        unsigned char low = (unsigned char)(number[i] & 0xf);

        *hex++ = (char)(high + (number[i] > 0x9f ? 7 : 0) + '0');
        *hex++ = (char)(low + (low > 9 ? 7 : 0) + '0');
    }
    *hex = 0;
}
