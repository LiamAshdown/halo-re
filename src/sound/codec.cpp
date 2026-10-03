/**
 * @file src/sound/codec.cpp
 * Xbox ADPCM decoding.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "internal/state.hpp"

namespace halo::sound {

namespace {

/** Advances an ADPCM step index by the nibble's table delta, clamped to the 89-entry step table. */
int32_t adpcm_next_index(int32_t index, uint32_t nibble)
{
    index = index + adpcm_index_table[nibble];
    if (index < 0) {
        return 0;
    }
    if (index >= 89) {
        return 88;
    }
    return index;
}

}  // namespace

namespace codec {

int32_t decode_dispatch(int16_t channel_count, void *destination, void *source, int32_t source_size)
{
    uint16_t nibble_bytes = (uint16_t)(((uint16_t)(((uint16_t)((uint16_t)(channel_count * 0xfc) + 7) >> 3) + 7)) >> 3);
    uint16_t block_size = (uint16_t)((channel_count + nibble_bytes * 2) << 2);
    int32_t block_count = source_size / (int32_t)block_size;
    int32_t decoded_size = (((int32_t)channel_count << 10) / 8) * block_count;
    int32_t out_0;
    int32_t out_1;

    if (destination == nullptr) {
        return decoded_size;
    }

    sound_decode_proc = k_sound_decode_procs[channel_count];
    out_0 = 0;
    out_1 = 0;
    if (sound_decode_proc(source, destination, block_count, (int32_t)block_size, 0x40, &out_0, &out_1) == 0) {
        return -1;
    }
    return 0;
}

int32_t decode_sample(uint8_t selector, int32_t prediction, uint32_t step)
{
    uint32_t negate;
    int32_t delta;

    negate = (uint32_t)((selector >> 3 & 1) != 0);
    delta = (int32_t)((((-(uint32_t)((selector & 4) != 0) & step) +
                        (-(uint32_t)((selector & 2) != 0) & (step >> 1)) +
                        (-(uint32_t)((selector & 1) != 0) & (step >> 2)) + (step >> 3)) ^ -negate) +
                       negate);
    prediction = (int32_t)((uint32_t)delta + (uint32_t)prediction);

    if (prediction < 0x8000) {
        if (prediction < -0x8000) {
            prediction = -0x8000;
        }
    } else {
        prediction = 0x7fff;
    }
    return prediction;
}

int32_t decode_mono(void *source, void *destination, int32_t block_count, int32_t block_size, int32_t samples_per_block, int32_t *out_0, int32_t *out_1)
{
    uint8_t *block = (uint8_t *)source;
    int16_t *out = (int16_t *)destination;
    uint32_t samples_after_header = (uint32_t)(samples_per_block - 1);

    if (block_count == 0) {
        return 1;
    }
    do {
        uint32_t header = *(uint32_t *)block;
        int32_t sample = (int16_t)header;
        int32_t index = (header >> 16) & 0xff;
        uint8_t *data = block + 4;
        uint32_t remaining = samples_after_header;

        block_count = block_count - 1;
        if (index >= 89) {
            return 0;
        }
        *out++ = (int16_t)sample;
        while (remaining != 0) {
            uint32_t byte = *data++;

            sample = codec::decode_sample((uint8_t)(byte & 0xf), sample, (uint32_t)adpcm_step_table[index]);
            index = adpcm_next_index(index, byte & 0xf);
            *out++ = (int16_t)sample;
            if (--remaining == 0) {
                break;
            }
            sample = codec::decode_sample((uint8_t)(byte >> 4), sample, (uint32_t)adpcm_step_table[index]);
            index = adpcm_next_index(index, byte >> 4);
            *out++ = (int16_t)sample;
            remaining = remaining - 1;
        }
        block = block + block_size;
    } while (block_count != 0);
    return 1;
}

int32_t decode_stereo(void *source, void *destination, int32_t block_count, int32_t block_size, int32_t samples_per_block, int32_t *out_0, int32_t *out_1)
{
    uint8_t *block = (uint8_t *)source;
    uint32_t *out = (uint32_t *)destination;
    uint32_t samples_after_header = (uint32_t)(samples_per_block - 1);

    if (block_count == 0) {
        return 1;
    }
    do {
        uint32_t left_header = ((uint32_t *)block)[0];
        uint32_t right_header;
        int32_t left_sample = (int16_t)left_header;
        int32_t left_index = (left_header >> 16) & 0xff;
        int32_t right_sample;
        int32_t right_index;
        uint32_t *data;
        uint32_t remaining = samples_after_header;

        block_count = block_count - 1;
        if (left_index >= 89) {
            return 0;
        }
        right_header = ((uint32_t *)block)[1];
        right_sample = (int16_t)right_header;
        right_index = (right_header >> 16) & 0xff;
        data = (uint32_t *)(block + 8);
        if (right_index >= 89) {
            return 0;
        }
        *out++ = ((uint32_t)(uint16_t)right_sample << 16) | (uint16_t)left_sample;
        while (remaining != 0) {
            uint32_t left_bits = data[0];
            uint32_t right_bits = data[1];
            uint32_t count = remaining < 8 ? remaining : 8;
            uint32_t i;

            data = data + 2;
            for (i = count; i != 0; i--) {
                left_sample = codec::decode_sample((uint8_t)(left_bits & 0xf), left_sample,
                    (uint32_t)adpcm_step_table[left_index]);
                left_index = adpcm_next_index(left_index, left_bits & 0xf);
                right_sample = codec::decode_sample((uint8_t)(right_bits & 0xf), right_sample,
                    (uint32_t)adpcm_step_table[right_index]);
                right_index = adpcm_next_index(right_index, right_bits & 0xf);
                *out++ = ((uint32_t)(uint16_t)right_sample << 16) | (uint16_t)left_sample;
                left_bits = left_bits >> 4;
                right_bits = right_bits >> 4;
            }
            remaining = remaining - count;
        }
        block = block + block_size;
    } while (block_count != 0);
    return 1;
}

}  // namespace codec


}  // namespace halo::sound
