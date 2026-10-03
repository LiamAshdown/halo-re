#include "halo/cseries/cseries.hpp"

#include "tags.h"
#include "memory.h"
#include "math.h"

#define MD5_F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | ~(z)))
#define MD5_ROTATE(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define MD5_STEP(f, a, b, c, d, x, s, t) { (a) += f((b), (c), (d)) + (x) + (uint32_t)(t); (a) = MD5_ROTATE((a), (s)); (a) += (b); }

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

    MD5_STEP(MD5_F, a, b, c, d, x[0], 7, 0xd76aa478); MD5_STEP(MD5_F, d, a, b, c, x[1], 12, 0xe8c7b756);
    MD5_STEP(MD5_F, c, d, a, b, x[2], 17, 0x242070db); MD5_STEP(MD5_F, b, c, d, a, x[3], 22, 0xc1bdceee);
    MD5_STEP(MD5_F, a, b, c, d, x[4], 7, 0xf57c0faf); MD5_STEP(MD5_F, d, a, b, c, x[5], 12, 0x4787c62a);
    MD5_STEP(MD5_F, c, d, a, b, x[6], 17, 0xa8304613); MD5_STEP(MD5_F, b, c, d, a, x[7], 22, 0xfd469501);
    MD5_STEP(MD5_F, a, b, c, d, x[8], 7, 0x698098d8); MD5_STEP(MD5_F, d, a, b, c, x[9], 12, 0x8b44f7af);
    MD5_STEP(MD5_F, c, d, a, b, x[10], 17, 0xffff5bb1); MD5_STEP(MD5_F, b, c, d, a, x[11], 22, 0x895cd7be);
    MD5_STEP(MD5_F, a, b, c, d, x[12], 7, 0x6b901122); MD5_STEP(MD5_F, d, a, b, c, x[13], 12, 0xfd987193);
    MD5_STEP(MD5_F, c, d, a, b, x[14], 17, 0xa679438e); MD5_STEP(MD5_F, b, c, d, a, x[15], 22, 0x49b40821);

    MD5_STEP(MD5_G, a, b, c, d, x[1], 5, 0xf61e2562); MD5_STEP(MD5_G, d, a, b, c, x[6], 9, 0xc040b340);
    MD5_STEP(MD5_G, c, d, a, b, x[11], 14, 0x265e5a51); MD5_STEP(MD5_G, b, c, d, a, x[0], 20, 0xe9b6c7aa);
    MD5_STEP(MD5_G, a, b, c, d, x[5], 5, 0xd62f105d); MD5_STEP(MD5_G, d, a, b, c, x[10], 9, 0x02441453);
    MD5_STEP(MD5_G, c, d, a, b, x[15], 14, 0xd8a1e681); MD5_STEP(MD5_G, b, c, d, a, x[4], 20, 0xe7d3fbc8);
    MD5_STEP(MD5_G, a, b, c, d, x[9], 5, 0x21e1cde6); MD5_STEP(MD5_G, d, a, b, c, x[14], 9, 0xc33707d6);
    MD5_STEP(MD5_G, c, d, a, b, x[3], 14, 0xf4d50d87); MD5_STEP(MD5_G, b, c, d, a, x[8], 20, 0x455a14ed);
    MD5_STEP(MD5_G, a, b, c, d, x[13], 5, 0xa9e3e905); MD5_STEP(MD5_G, d, a, b, c, x[2], 9, 0xfcefa3f8);
    MD5_STEP(MD5_G, c, d, a, b, x[7], 14, 0x676f02d9); MD5_STEP(MD5_G, b, c, d, a, x[12], 20, 0x8d2a4c8a);

    MD5_STEP(MD5_H, a, b, c, d, x[5], 4, 0xfffa3942); MD5_STEP(MD5_H, d, a, b, c, x[8], 11, 0x8771f681);
    MD5_STEP(MD5_H, c, d, a, b, x[11], 16, 0x6d9d6122); MD5_STEP(MD5_H, b, c, d, a, x[14], 23, 0xfde5380c);
    MD5_STEP(MD5_H, a, b, c, d, x[1], 4, 0xa4beea44); MD5_STEP(MD5_H, d, a, b, c, x[4], 11, 0x4bdecfa9);
    MD5_STEP(MD5_H, c, d, a, b, x[7], 16, 0xf6bb4b60); MD5_STEP(MD5_H, b, c, d, a, x[10], 23, 0xbebfbc70);
    MD5_STEP(MD5_H, a, b, c, d, x[13], 4, 0x289b7ec6); MD5_STEP(MD5_H, d, a, b, c, x[0], 11, 0xeaa127fa);
    MD5_STEP(MD5_H, c, d, a, b, x[3], 16, 0xd4ef3085); MD5_STEP(MD5_H, b, c, d, a, x[6], 23, 0x04881d05);
    MD5_STEP(MD5_H, a, b, c, d, x[9], 4, 0xd9d4d039); MD5_STEP(MD5_H, d, a, b, c, x[12], 11, 0xe6db99e5);
    MD5_STEP(MD5_H, c, d, a, b, x[15], 16, 0x1fa27cf8); MD5_STEP(MD5_H, b, c, d, a, x[2], 23, 0xc4ac5665);

    MD5_STEP(MD5_I, a, b, c, d, x[0], 6, 0xf4292244); MD5_STEP(MD5_I, d, a, b, c, x[7], 10, 0x432aff97);
    MD5_STEP(MD5_I, c, d, a, b, x[14], 15, 0xab9423a7); MD5_STEP(MD5_I, b, c, d, a, x[5], 21, 0xfc93a039);
    MD5_STEP(MD5_I, a, b, c, d, x[12], 6, 0x655b59c3); MD5_STEP(MD5_I, d, a, b, c, x[3], 10, 0x8f0ccc92);
    MD5_STEP(MD5_I, c, d, a, b, x[10], 15, 0xffeff47d); MD5_STEP(MD5_I, b, c, d, a, x[1], 21, 0x85845dd1);
    MD5_STEP(MD5_I, a, b, c, d, x[8], 6, 0x6fa87e4f); MD5_STEP(MD5_I, d, a, b, c, x[15], 10, 0xfe2ce6e0);
    MD5_STEP(MD5_I, c, d, a, b, x[6], 15, 0xa3014314); MD5_STEP(MD5_I, b, c, d, a, x[13], 21, 0x4e0811a1);
    MD5_STEP(MD5_I, a, b, c, d, x[4], 6, 0xf7537e82); MD5_STEP(MD5_I, d, a, b, c, x[11], 10, 0xbd3af235);
    MD5_STEP(MD5_I, c, d, a, b, x[2], 15, 0x2ad7d2bb); MD5_STEP(MD5_I, b, c, d, a, x[9], 21, 0xeb86d391);

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
    context.state[0] = 0x67452301;
    context.state[1] = 0xefcdab89;
    context.state[2] = 0x98badcfe;
    context.state[3] = 0x10325476;
    context.update(data, (uint32_t)length);
    context.finish(digest);
    halo::cseries::md5_context::to_hex(digest, out);
}

} // namespace halo::cseries
