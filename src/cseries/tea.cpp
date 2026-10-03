#include "halo/cseries/cseries.hpp"

#include "tags.h"
#include "memory.h"
#include "math.h"

namespace halo::cseries {

/**
 * Encrypts one 8 byte block in place with 32 TEA rounds.
 *
 * @address 0x6181c0
 */
void tea_key::encrypt_block(uint32_t *block) const
{
    uint32_t v0 = block[0];
    uint32_t v1 = block[1];
    uint32_t sum = 0;
    int32_t round;

    for (round = 32; round != 0; round--) {
        sum += 0x9e3779b9;
        v0 += ((v1 << 4) + this->words[0]) ^ (v1 + sum) ^ ((v1 >> 5) + this->words[1]);
        v1 += ((v0 << 4) + this->words[2]) ^ (v0 + sum) ^ ((v0 >> 5) + this->words[3]);
    }
    block[0] = v0;
    block[1] = v1;
}

/**
 * Decrypts one 8 byte block in place; the inverse of encrypt_block.
 *
 * @address 0x6182b0
 */
void tea_key::decrypt_block(uint32_t *block) const
{
    uint32_t v0 = block[0];
    uint32_t v1 = block[1];
    uint32_t sum = 0xc6ef3720;
    int32_t round;

    for (round = 0; round < 32; round++) {
        v1 -= ((v0 << 4) + this->words[2]) ^ (v0 + sum) ^ ((v0 >> 5) + this->words[3]);
        v0 -= ((v1 << 4) + this->words[0]) ^ (v1 + sum) ^ ((v1 >> 5) + this->words[1]);
        sum += 0x61c88647;
    }
    block[0] = v0;
    block[1] = v1;
}

/**
 * Encrypts every full 8 byte block of data in order and then, when length is not a multiple of 8, the
 * last 8 bytes again (overlapping the final block). Buffers shorter than 8 bytes are left alone.
 *
 * @address 0x618250
 */
void tea_key::encrypt_buffer(int32_t length, uint8_t *data) const
{
    int32_t blocks;
    uint8_t *block;

    if (length < 8) {
        return;
    }
    block = data;
    for (blocks = length / 8; blocks > 0; blocks--) {
        this->encrypt_block((uint32_t *)block);
        block += 8;
    }
    if (length % 8 != 0) {
        this->encrypt_block((uint32_t *)(data + length - 8));
    }
}

/**
 * Decrypts a buffer produced by encrypt_buffer: the overlapping last block first, then every full
 * block in order.
 *
 * @address 0x618350
 */
void tea_key::decrypt_buffer(int32_t length, uint8_t *data) const
{
    int32_t blocks;

    if (length < 8) {
        return;
    }
    if (length % 8 != 0) {
        this->decrypt_block((uint32_t *)(data + length - 8));
    }
    for (blocks = length / 8; blocks > 0; blocks--) {
        this->decrypt_block((uint32_t *)data);
        data += 8;
    }
}

} // namespace halo::cseries
