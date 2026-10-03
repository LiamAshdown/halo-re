#include "halo/sound/vorbis_stream.hpp"

#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.c"

namespace halo::sound {

/**
 * Opens a decoder over the in-memory file and records its channel count and sample rate.
 */
int32_t vorbis_stream_open(VorbisStream *stream, const void *data, int32_t size)
{
    int error = 0;
    stb_vorbis *decoder = stb_vorbis_open_memory(static_cast<const unsigned char *>(data), size, &error, nullptr);

    if (decoder == nullptr) {
        stream->decoder = nullptr;
        return error != 0 ? -error : -1;
    }
    const stb_vorbis_info info = stb_vorbis_get_info(decoder);
    stream->decoder = decoder;
    stream->channels = info.channels;
    stream->sample_rate = static_cast<int32_t>(info.sample_rate);
    return 0;
}

/**
 * Decodes whole sample frames into the buffer; a frame is channels * 2 bytes.
 */
int32_t vorbis_stream_read(VorbisStream *stream, char *buffer, int32_t byte_count)
{
    if (stream->decoder == nullptr || stream->channels <= 0) {
        return -1;
    }
    const int32_t shorts = byte_count / 2;
    const int32_t frames = stb_vorbis_get_samples_short_interleaved(static_cast<stb_vorbis *>(stream->decoder), stream->channels,
                                                                    reinterpret_cast<short *>(buffer), shorts);
    return frames * stream->channels * 2;
}

/**
 * Frees the decoder state.
 */
void vorbis_stream_close(VorbisStream *stream)
{
    if (stream->decoder != nullptr) {
        stb_vorbis_close(static_cast<stb_vorbis *>(stream->decoder));
        stream->decoder = nullptr;
    }
}

}  // namespace halo::sound
