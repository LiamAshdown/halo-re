/**
 * @file include/halo/sound/ogg_stream.hpp
 * Ogg Vorbis memory streams and PCM feed from the sound cache.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::stream {

/**
 * Copies up to `requested_size` bytes of raw PCM from `permutation`'s cached sample pool (bounds-checked
 * against the sound cache's reserved memory region) into `destination`, advancing `*position` and reporting the
 * number of bytes actually copied through `*bytes_read_out`. Returns the number of bytes remaining in the
 * permutation's buffer after this read. Logs an error and copies nothing if the cached sample pointer falls
 * outside the sound cache.
 *
 * @address 0x00545860
 */
int32_t pcm_buffer_read(uint32_t *position, SoundPermutation *permutation, uint32_t *bytes_read_out, uint32_t requested_size, void *destination);

}  // namespace halo::sound::stream
