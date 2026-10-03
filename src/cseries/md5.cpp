#include "halo/cseries/cseries.hpp"

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "halo/cseries/api.hpp"

namespace {

constexpr uint32_t k_md5_table[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

constexpr uint32_t k_md5_initial_state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};

constexpr uint32_t md5_f(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
constexpr uint32_t md5_g(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
constexpr uint32_t md5_h(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
constexpr uint32_t md5_i(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }
constexpr uint32_t md5_rotate(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

inline void md5_step(uint32_t (*f)(uint32_t, uint32_t, uint32_t), uint32_t &a, uint32_t b, uint32_t c, uint32_t d,
                     uint32_t x, uint32_t shift, uint32_t constant)
{
    a += f(b, c, d) + x + constant;
    a = md5_rotate(a, shift);
    a += b;
}

}

extern "C" {
extern const uint8_t md5_padding[64];
extern int32_t sprintf(char *buffer, const char *format, ...);
extern const char md5_hex_byte_format[];
}

namespace halo::cseries {

/**
 * Feeds length bytes into the MD5 state: updates the bit count, tops up and transforms the 64 byte
 * buffer, transforms every further full block directly from the input and keeps the remainder in the
 * buffer.
 *
 * @address 0x61a5a0
 */
void md5_context::update(const uint8_t *input, uint32_t length)
{
    uint32_t index = (this->count[0] >> 3) & 0x3f;
    uint32_t part_length = 64 - index;
    uint32_t i;
    uint32_t j;

    this->count[0] += length << 3;
    if (this->count[0] < (length << 3)) {
        this->count[1]++;
    }
    this->count[1] += length >> 29;

    if (length >= part_length) {
        for (j = 0; j < part_length; j++) {
            this->buffer[index + j] = input[j];
        }
        this->transform(this->buffer);
        for (i = part_length; i + 63 < length; i += 64) {
            this->transform(&input[i]);
        }
        index = 0;
    } else {
        i = 0;
    }
    for (j = 0; j < length - i; j++) {
        this->buffer[index + j] = input[i + j];
    }
}

/**
 * Pads the message to 56 modulo 64 bytes, appends the 64 bit length, writes the state little-endian
 * into digest and zeroes the whole context.
 *
 * @address 0x61a660
 */
void md5_context::finish(uint8_t *digest)
{
    uint8_t bits[8];
    uint32_t index;
    int32_t i;

    for (i = 0; i < 8; i++) {
        bits[i] = (uint8_t)(this->count[i / 4] >> ((i % 4) * 8));
    }
    index = (this->count[0] >> 3) & 0x3f;
    this->update(md5_padding, index < 56 ? 56 - index : 120 - index);
    this->update(bits, 8);
    for (i = 0; i < 16; i++) {
        digest[i] = (uint8_t)(this->state[i / 4] >> ((i % 4) * 8));
    }
    for (i = 0; i < (int32_t)(sizeof(md5_context) / 4); i++) {
        ((uint32_t *)this)[i] = 0;
    }
}

/**
 * Runs the four unrolled MD5 rounds over one 64 byte block (decoded little-endian) and adds the result
 * into the state.
 *
 * @address 0x619cc0
 */
void md5_context::transform(const uint8_t *block)
{
    uint32_t a = this->state[0];
    uint32_t b = this->state[1];
    uint32_t c = this->state[2];
    uint32_t d = this->state[3];
    uint32_t x[16];
    int32_t i;

    for (i = 0; i < 16; i++) {
        x[i] = (uint32_t)block[i * 4] | ((uint32_t)block[i * 4 + 1] << 8) | ((uint32_t)block[i * 4 + 2] << 16) |
               ((uint32_t)block[i * 4 + 3] << 24);
    }

    md5_step(md5_f, a, b, c, d, x[0], 7, k_md5_table[0]); md5_step(md5_f, d, a, b, c, x[1], 12, k_md5_table[1]);
    md5_step(md5_f, c, d, a, b, x[2], 17, k_md5_table[2]); md5_step(md5_f, b, c, d, a, x[3], 22, k_md5_table[3]);
    md5_step(md5_f, a, b, c, d, x[4], 7, k_md5_table[4]); md5_step(md5_f, d, a, b, c, x[5], 12, k_md5_table[5]);
    md5_step(md5_f, c, d, a, b, x[6], 17, k_md5_table[6]); md5_step(md5_f, b, c, d, a, x[7], 22, k_md5_table[7]);
    md5_step(md5_f, a, b, c, d, x[8], 7, k_md5_table[8]); md5_step(md5_f, d, a, b, c, x[9], 12, k_md5_table[9]);
    md5_step(md5_f, c, d, a, b, x[10], 17, k_md5_table[10]); md5_step(md5_f, b, c, d, a, x[11], 22, k_md5_table[11]);
    md5_step(md5_f, a, b, c, d, x[12], 7, k_md5_table[12]); md5_step(md5_f, d, a, b, c, x[13], 12, k_md5_table[13]);
    md5_step(md5_f, c, d, a, b, x[14], 17, k_md5_table[14]); md5_step(md5_f, b, c, d, a, x[15], 22, k_md5_table[15]);

    md5_step(md5_g, a, b, c, d, x[1], 5, k_md5_table[16]); md5_step(md5_g, d, a, b, c, x[6], 9, k_md5_table[17]);
    md5_step(md5_g, c, d, a, b, x[11], 14, k_md5_table[18]); md5_step(md5_g, b, c, d, a, x[0], 20, k_md5_table[19]);
    md5_step(md5_g, a, b, c, d, x[5], 5, k_md5_table[20]); md5_step(md5_g, d, a, b, c, x[10], 9, k_md5_table[21]);
    md5_step(md5_g, c, d, a, b, x[15], 14, k_md5_table[22]); md5_step(md5_g, b, c, d, a, x[4], 20, k_md5_table[23]);
    md5_step(md5_g, a, b, c, d, x[9], 5, k_md5_table[24]); md5_step(md5_g, d, a, b, c, x[14], 9, k_md5_table[25]);
    md5_step(md5_g, c, d, a, b, x[3], 14, k_md5_table[26]); md5_step(md5_g, b, c, d, a, x[8], 20, k_md5_table[27]);
    md5_step(md5_g, a, b, c, d, x[13], 5, k_md5_table[28]); md5_step(md5_g, d, a, b, c, x[2], 9, k_md5_table[29]);
    md5_step(md5_g, c, d, a, b, x[7], 14, k_md5_table[30]); md5_step(md5_g, b, c, d, a, x[12], 20, k_md5_table[31]);

    md5_step(md5_h, a, b, c, d, x[5], 4, k_md5_table[32]); md5_step(md5_h, d, a, b, c, x[8], 11, k_md5_table[33]);
    md5_step(md5_h, c, d, a, b, x[11], 16, k_md5_table[34]); md5_step(md5_h, b, c, d, a, x[14], 23, k_md5_table[35]);
    md5_step(md5_h, a, b, c, d, x[1], 4, k_md5_table[36]); md5_step(md5_h, d, a, b, c, x[4], 11, k_md5_table[37]);
    md5_step(md5_h, c, d, a, b, x[7], 16, k_md5_table[38]); md5_step(md5_h, b, c, d, a, x[10], 23, k_md5_table[39]);
    md5_step(md5_h, a, b, c, d, x[13], 4, k_md5_table[40]); md5_step(md5_h, d, a, b, c, x[0], 11, k_md5_table[41]);
    md5_step(md5_h, c, d, a, b, x[3], 16, k_md5_table[42]); md5_step(md5_h, b, c, d, a, x[6], 23, k_md5_table[43]);
    md5_step(md5_h, a, b, c, d, x[9], 4, k_md5_table[44]); md5_step(md5_h, d, a, b, c, x[12], 11, k_md5_table[45]);
    md5_step(md5_h, c, d, a, b, x[15], 16, k_md5_table[46]); md5_step(md5_h, b, c, d, a, x[2], 23, k_md5_table[47]);

    md5_step(md5_i, a, b, c, d, x[0], 6, k_md5_table[48]); md5_step(md5_i, d, a, b, c, x[7], 10, k_md5_table[49]);
    md5_step(md5_i, c, d, a, b, x[14], 15, k_md5_table[50]); md5_step(md5_i, b, c, d, a, x[5], 21, k_md5_table[51]);
    md5_step(md5_i, a, b, c, d, x[12], 6, k_md5_table[52]); md5_step(md5_i, d, a, b, c, x[3], 10, k_md5_table[53]);
    md5_step(md5_i, c, d, a, b, x[10], 15, k_md5_table[54]); md5_step(md5_i, b, c, d, a, x[1], 21, k_md5_table[55]);
    md5_step(md5_i, a, b, c, d, x[8], 6, k_md5_table[56]); md5_step(md5_i, d, a, b, c, x[15], 10, k_md5_table[57]);
    md5_step(md5_i, c, d, a, b, x[6], 15, k_md5_table[58]); md5_step(md5_i, b, c, d, a, x[13], 21, k_md5_table[59]);
    md5_step(md5_i, a, b, c, d, x[4], 6, k_md5_table[60]); md5_step(md5_i, d, a, b, c, x[11], 10, k_md5_table[61]);
    md5_step(md5_i, c, d, a, b, x[2], 15, k_md5_table[62]); md5_step(md5_i, b, c, d, a, x[9], 21, k_md5_table[63]);

    this->state[0] += a;
    this->state[1] += b;
    this->state[2] += c;
    this->state[3] += d;
}

/**
 * Formats the 16 byte digest as 32 lowercase hex digits plus a terminator in out.
 *
 * @address 0x619c90
 */
void md5_context::to_hex(const uint8_t *digest, char *out)
{
    uint32_t i;

    for (i = 0; i < 0x10; i++) {
        sprintf(out, md5_hex_byte_format, digest[i]);
        out += 2;
    }
}

/**
 * Hashes length bytes of data with a fresh context and writes the 32 character hex digest to out. Used
 * to verify shader resource files.
 *
 * @address 0x61a730
 */
void md5_context::hex_digest(const uint8_t *data, int32_t length, char *out)
{
    md5_context context;
    uint8_t digest[16];

    context.count[0] = 0;
    context.count[1] = 0;
    for (int32_t i = 0; i < 4; i++) {
        context.state[i] = k_md5_initial_state[i];
    }
    context.update(data, (uint32_t)length);
    context.finish(digest);
    halo::cseries::md5_context::to_hex(digest, out);
}

} // namespace halo::cseries
