/**
 * @file include/halo/sound/vorbis_stream.hpp
 * Ogg Vorbis decoding for the sound streams: a VorbisStream decodes an in-memory Ogg Vorbis file to interleaved 16-bit
 * little-endian PCM. It replaces the vorbisfile library the game used to load from the install folder.
 */
#pragma once

#include <cstdint>

namespace halo::sound {

/** A decoder instance over one in-memory Ogg Vorbis file. Lives in the sound stream's per-slot storage. */
struct VorbisStream {
    void *decoder;       // the underlying decoder state, null when closed
    int32_t channels;
    int32_t sample_rate;
};

/** Bytes the sound stream reserves for each slot's decoder state; VorbisStream must fit in it. */
inline constexpr uint32_t k_vorbis_stream_storage_size = 0x2d0;
static_assert(sizeof(VorbisStream) <= k_vorbis_stream_storage_size);

/**
 * Starts decoding the Ogg Vorbis file that occupies `size` bytes at `data`; the memory must stay valid until close.
 * Returns 0 on success, a negative value when the data is not a decodable Ogg Vorbis stream.
 */
int32_t vorbis_stream_open(VorbisStream *stream, const void *data, int32_t size);

/**
 * Decodes up to `byte_count` bytes of PCM into `buffer`, whole sample frames only. Returns the bytes written, 0 at the end
 * of the stream, or a negative value on a decode failure.
 */
int32_t vorbis_stream_read(VorbisStream *stream, char *buffer, int32_t byte_count);

/** Releases the decoder; the stream can be opened again afterwards. Safe on a stream that was never opened. */
void vorbis_stream_close(VorbisStream *stream);

}  // namespace halo::sound
