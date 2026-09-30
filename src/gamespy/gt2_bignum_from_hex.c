// gt2_bignum_from_hex  (GameSpy SDK in halo.exe; no C existed)
// address 0x617fb0, size 135 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617fb0..0x618036: zeroes the gt2_bignum_length-byte number, then for each
//   character while it is positive (signed): shifts the number left four bits and ORs in the digit. The character is
//   lower-cased IN PLACE (| 0x20) and its value is c - 0x30, less 0x27 more when above 0x60.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

extern unsigned int gt2_bignum_length;          // 0x00683944, bytes per big number
extern unsigned char gt2_bignum_modulus[0x400]; // 0x00723240

void gt2_bignum_from_hex(char *hex, unsigned char *number)
{
    int shift;
    int carry;
    unsigned int i;

    memset(number, 0, gt2_bignum_length);
    while (*hex > 0) {
        char c;

        for (shift = 4; shift != 0; shift--) {
            carry = 0;
            for (i = gt2_bignum_length; i != 0; i--) {
                carry += number[i - 1] * 2;
                number[i - 1] = (unsigned char)carry;
                carry >>= 8;
            }
        }
        c = (char)(*hex | 0x20);
        *hex = c;
        number[gt2_bignum_length - 1] |= (unsigned char)(c - (c > 0x60 ? 0x27 : 0) - 0x30);
        hex++;
    }
}
