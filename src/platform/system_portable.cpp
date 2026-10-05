/**
 * @file src/platform/system_portable.cpp
 * The system services that are plain computation (SHA-1, Windows-1252 <-> UTF-16), compiled on every platform so
 * tools/platform_test.cpp can check them against the Windows implementations; src/platform/system_posix.cpp uses them.
 */

#include "system_portable.hpp"

#include <string.h>

namespace halo::platform::portable {

namespace {

/** Windows-1252 0x80..0x9f (0 = unassigned, mapped like Windows to the same code point). */
const uint16_t k_cp1252_high[32] = {
    0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178,
};

uint32_t rotate(uint32_t value, int bits)
{
    return (value << bits) | (value >> (32 - bits));
}

void sha1_block(uint32_t state[5], const uint8_t *block)
{
    uint32_t w[80];
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];

    for (int i = 0; i < 16; i++) {
        w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) | (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(block[i * 4 + 2]) << 8) | block[i * 4 + 3];
    }
    for (int i = 16; i < 80; i++) {
        w[i] = rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }
    for (int i = 0; i < 80; i++) {
        uint32_t f, k, t;

        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5a827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ed9eba1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8f1bbcdc;
        } else {
            f = b ^ c ^ d;
            k = 0xca62c1d6;
        }
        t = rotate(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotate(b, 30);
        b = a;
        a = t;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

}  // namespace

void sha1(const uint8_t *data, uint32_t size, uint8_t digest[20])
{
    uint32_t state[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    uint8_t tail[128];
    uint32_t full = size & ~63u;
    uint32_t rest = size - full;
    uint32_t tail_size = rest < 56 ? 64 : 128;
    uint64_t bits = static_cast<uint64_t>(size) * 8;

    for (uint32_t offset = 0; offset < full; offset += 64) {
        sha1_block(state, data + offset);
    }
    memset(tail, 0, sizeof(tail));
    memcpy(tail, data + full, rest);
    tail[rest] = 0x80;
    for (int i = 0; i < 8; i++) {
        tail[tail_size - 1 - i] = static_cast<uint8_t>(bits >> (8 * i));
    }
    sha1_block(state, tail);
    if (tail_size == 128) {
        sha1_block(state, tail + 64);
    }
    for (int i = 0; i < 5; i++) {
        digest[i * 4] = static_cast<uint8_t>(state[i] >> 24);
        digest[i * 4 + 1] = static_cast<uint8_t>(state[i] >> 16);
        digest[i * 4 + 2] = static_cast<uint8_t>(state[i] >> 8);
        digest[i * 4 + 3] = static_cast<uint8_t>(state[i]);
    }
}

int32_t ansi_to_wide(const char *text, int32_t length, uint16_t *out, int32_t capacity)
{
    int32_t count = length < 0 ? static_cast<int32_t>(strlen(text)) + 1 : length;

    if (capacity == 0) {
        return count;  // the size needed
    }
    if (capacity < count) {
        return 0;  // ERROR_INSUFFICIENT_BUFFER
    }
    for (int32_t i = 0; i < count; i++) {
        uint8_t c = static_cast<uint8_t>(text[i]);

        out[i] = c >= 0x80 && c < 0xa0 ? k_cp1252_high[c - 0x80] : c;
    }
    return count;
}

int32_t wide_to_ansi(const uint16_t *text, int32_t length, char *out, int32_t capacity)
{
    int32_t count = 0;

    if (length < 0) {
        while (text[count] != 0) count++;
        length = count + 1;
    }
    if (capacity == 0) {
        return length;
    }
    if (capacity < length) {
        return 0;
    }
    for (int32_t i = 0; i < length; i++) {
        uint16_t c = text[i];
        char converted = '?';

        if (c < 0x80 || (c >= 0xa0 && c < 0x100)) {
            converted = static_cast<char>(c);
        } else {
            for (int k = 0; k < 32; k++) {
                if (k_cp1252_high[k] == c) {
                    converted = static_cast<char>(0x80 + k);
                    break;
                }
            }
        }
        out[i] = converted;
    }
    return length;
}

}  // namespace halo::platform::portable
