/**
 * @file include/halo/sound/ogg_stream.hpp
 * Ogg Vorbis memory streams and PCM feed from the sound cache.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::stream {

/**
 * libvorbisfile read callback over an in-memory stream: copies up to size * count bytes from the current
 * position, setting the end-of-file flag when the data runs out.
 *
 * @address 0x00544d50
 */
uint32_t read_memory_file(void *destination, uint32_t size, uint32_t count, sound_ogg_memory_file *file);

/**
 * libvorbisfile seek callback; forwards to the memory file's seek.
 *
 * @address 0x00544da0
 */
int32_t seek_memory_file(sound_ogg_memory_file *file, uint32_t offset_low, int32_t offset_high, int32_t whence);

/**
 * libvorbisfile close callback: detaches the data pointer. Returns -1 for a null stream.
 *
 * @address 0x00544dc0
 */
int32_t close_memory_file(sound_ogg_memory_file *file);

/**
 * libvorbisfile tell callback: returns the stream position, or its size once the end of file was reached.
 * Returns -1 for a null stream.
 *
 * @address 0x00544de0
 */
int32_t tell_memory_file(sound_ogg_memory_file *file);

/**
 * Formats a libvorbisfile error code into a human-readable message in a scratch buffer.
 *
 * @address 0x00544f70
 */
void error_to_string(int32_t vorbis_error_code);

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
