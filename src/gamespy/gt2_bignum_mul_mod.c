// gt2_bignum_mul_mod  (GameSpy SDK in halo.exe; no C existed)
// address 0x617db0, size 505 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617db0..0x617fa8: a = a * b mod m for big-endian byte strings of
//   gt2_bignum_length bytes (m the global modulus at 0x723240): shift-and-add over a copy of a (from its low bit)
//   with a doubled copy of b, each partial sum and doubling reduced by one subtraction when not below m (the first
//   differing byte of the first n-1 decides; the last byte is compared unsigned). Carries and borrows are arithmetic
//   shifts.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

extern unsigned int gt2_bignum_length;          // 0x00683944, bytes per big number
extern unsigned char gt2_bignum_modulus[0x400]; // 0x00723240

void gt2_bignum_mul_mod(unsigned char *a, const unsigned char *b)
{
    unsigned char x[0x400];
    unsigned char y[0x400];
    unsigned int n = gt2_bignum_length;
    int bits;
    int carry;
    unsigned int i;
    int j;

    memcpy(x, a, n);
    memcpy(y, b, n);
    memset(a, 0, n);
    for (bits = (int)(gt2_bignum_length * 8); bits != 0; bits--) {
        n = gt2_bignum_length;
        if (x[n - 1] & 1) {
            carry = 0;
            for (i = n; i != 0; i--) {
                carry = a[i - 1] + carry + y[i - 1];
                a[i - 1] = (unsigned char)carry;
                carry >>= 8;
            }
            n = gt2_bignum_length;
            for (j = 0; j < (int)(n - 1) && a[j] == gt2_bignum_modulus[j]; j++) {
            }
            if (a[j] >= gt2_bignum_modulus[j]) {
                carry = 0;
                for (i = n; i != 0; i--) {
                    carry += a[i - 1] - gt2_bignum_modulus[i - 1];
                    a[i - 1] = (unsigned char)carry;
                    carry >>= 8;
                }
                n = gt2_bignum_length;
            }
        }
        carry = 0;
        for (j = 0; j < (int)n; j++) {
            carry |= x[j];
            x[j] = (unsigned char)(carry / 2);
            carry = (carry & 1) << 8;
        }
        carry = 0;
        for (i = n; i != 0; i--) {
            carry += y[i - 1] * 2;
            y[i - 1] = (unsigned char)carry;
            carry >>= 8;
        }
        for (j = 0; j < (int)(n - 1) && y[j] == gt2_bignum_modulus[j]; j++) {
        }
        if (y[j] >= gt2_bignum_modulus[j]) {
            carry = 0;
            for (i = n; i != 0; i--) {
                carry += y[i - 1] - gt2_bignum_modulus[i - 1];
                y[i - 1] = (unsigned char)carry;
                carry >>= 8;
            }
        }
    }
}
