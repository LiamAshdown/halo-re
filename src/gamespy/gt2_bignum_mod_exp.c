// gt2_bignum_mod_exp  (GameSpy SDK in halo.exe; no C existed)
// address 0x618040, size 272 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618040..0x61814f: result = base ^ exponent mod modulus, all three given in hex
//   (parsed with gt2_bignum_from_hex, the modulus into the global at 0x723240; the strings get lower-cased): result
//   starts at 1, then for every bit from the exponent  low end: multiply in the base when set, square the base, shift
//   the exponent right.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

extern unsigned int gt2_bignum_length;          // 0x00683944, bytes per big number
extern unsigned char gt2_bignum_modulus[0x400]; // 0x00723240


void gt2_bignum_mod_exp(char *base_hex, char *exponent_hex, char *modulus_hex, unsigned char *result)
{
    unsigned char base[0x400];
    unsigned char exponent[0x400];
    unsigned int n;
    int bits;
    int carry;
    int j;

    gt2_bignum_from_hex(base_hex, base);
    gt2_bignum_from_hex(exponent_hex, exponent);
    gt2_bignum_from_hex(modulus_hex, gt2_bignum_modulus);
    memset(result, 0, gt2_bignum_length);
    result[gt2_bignum_length - 1] = 1;
    n = gt2_bignum_length;
    for (bits = (int)(gt2_bignum_length * 8); bits != 0; bits--) {
        if (exponent[n - 1] & 1) {
            gt2_bignum_mul_mod(result, base);
            n = gt2_bignum_length;
        }
        gt2_bignum_mul_mod(base, base);
        carry = 0;
        for (j = 0; j < (int)n; j++) {
            carry |= exponent[j];
            exponent[j] = (unsigned char)(carry / 2);
            carry = (carry & 1) << 8;
        }
    }
}
