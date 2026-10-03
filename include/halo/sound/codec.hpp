/**
 * @file include/halo/sound/codec.hpp
 * Xbox ADPCM decoding.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::codec {

/**
 * Decodes `source_size` bytes of Xbox ADPCM into `destination`: returns 0 on success, -1 when the decoder
 * fails; with no destination, returns the number of bytes the decode would produce.
 *
 * @address 0x0054e830
 */
int32_t decode_dispatch(int16_t channel_count, void *destination, void *source, int32_t source_size);

/**
 * Decodes one 4-bit Xbox ADPCM nibble against the current prediction and step size, returning the new
 * prediction clamped to 16 bits.
 *
 * @address 0x0054e8c0
 */
int32_t decode_sample(uint8_t selector, int32_t prediction, uint32_t step);

/**
 * Decodes `block_count` mono Xbox ADPCM blocks into 16-bit PCM. Returns 0 when a block header carries an
 * out-of-range step index, otherwise 1.
 *
 * @address 0x0054e920
 */
int32_t decode_mono(void *source, void *destination, int32_t block_count, int32_t block_size, int32_t samples_per_block, int32_t *out_0, int32_t *out_1);

/**
 * Decodes `block_count` interleaved stereo Xbox ADPCM blocks into 16-bit PCM pairs. Returns 0 when a block
 * header carries an out-of-range step index, otherwise 1.
 *
 * @address 0x0054ea60
 */
int32_t decode_stereo(void *source, void *destination, int32_t block_count, int32_t block_size, int32_t samples_per_block, int32_t *out_0, int32_t *out_1);

}  // namespace halo::sound::codec
