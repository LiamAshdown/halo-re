/**
 * @file src/sound/ogg_stream.cpp
 * Ogg Vorbis memory streams and PCM feed from the sound cache.
 */

#include "internal/state.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/vorbis_stream.hpp"

namespace halo::sound {

namespace {

/** Closes a slot's decoder and zeroes its storage. */
void sound_stream_decoder_clear_ogg_vorbis_file(void *vorbis_file)
{
    uint32_t *words = (uint32_t *)vorbis_file;
    int32_t i;

    vorbis_stream_close(static_cast<VorbisStream *>(vorbis_file));
    for (i = 0; i < (int32_t)(k_ogg_vorbis_file_size / 4); i++) {
        words[i] = 0;
    }
}

}  // namespace

namespace stream {

int32_t pcm_buffer_read(uint32_t *position, SoundPermutation *permutation, uint32_t *bytes_read_out, uint32_t requested_size, void *destination)
{
    uint32_t remaining = permutation->buffer_size - *position;
    uint32_t sample_pointer = *(uint32_t *)&((struct SoundPermutation *)permutation)->cache_page;

    if ((int32_t)requested_size <= (int32_t)remaining) {
        remaining = requested_size;
    }
    *bytes_read_out = remaining;

    if (sample_pointer >= (uint32_t)halo::cache::globals().sound_cache_memory &&
        permutation->samples.size + sample_pointer <=
            ((uint32_t)(int32_t)*(int16_t *)&sound_cache_size_megabytes << 20) + (uint32_t)halo::cache::globals().sound_cache_memory) {
        uint8_t *src = (uint8_t *)(sample_pointer + (*position & 0xfffffffeu));
        uint8_t *dst = (uint8_t *)destination;
        uint32_t new_position;

        memcpy(dst, src, remaining);

        new_position = *position + *bytes_read_out;
        *position = new_position;
        return (int32_t)(permutation->buffer_size - new_position);
    }

    if (strlen(error_text_buffer) < 0xd6) {
        sprintf(error_text_buffer + strlen(error_text_buffer), "trying to queue sound but samples is null.");
    }

    return (int32_t)(permutation->buffer_size - *position);
}

}  // namespace stream

uint8_t StreamDecoder::open_stream(void *data, int32_t size)
{
    sound_ogg_memory_file *memory_file;
    void *ogg_vorbis_file;
    int32_t result;

    if (this->active_file == 0) {
        memory_file = &this->memory_files[1];
        ogg_vorbis_file = this->ogg_vorbis_file[1];
    } else {
        memory_file = &this->memory_files[0];
        ogg_vorbis_file = this->ogg_vorbis_file[0];
    }

    memory_file->position = 0;
    memory_file->end_of_file = 0;
    memory_file->data = data;
    memory_file->size = size;

    this->decoded_bytes = 0;

    result = vorbis_stream_open(static_cast<VorbisStream *>(ogg_vorbis_file), data, size);

    if (result < 0) {
        return 0;
    }

    this->open = 1;
    return 1;
}

int32_t StreamDecoder::read_stream(char *buffer, int32_t size, char *want_crosslap)
{
    int32_t total = 0;
    uint8_t crosslapped = 0;
    void *vorbis_file;
    int32_t read_result;

    for (;;) {
        if (*want_crosslap != 0 && crosslapped == 0) {
            void *other = (this->active_file == 0) ? this->ogg_vorbis_file[0] : this->ogg_vorbis_file[1];

            vorbis_stream_close(static_cast<VorbisStream *>(other));
            crosslapped = 1;
        }

        vorbis_file = (this->active_file == 0) ? this->ogg_vorbis_file[1] : this->ogg_vorbis_file[0];

        read_result = vorbis_stream_read(static_cast<VorbisStream *>(vorbis_file), buffer + total, size);
        if (read_result < 1) {
            break;
        }

        size -= read_result;
        total += read_result;
        if (size == 0) {
            this->decoded_bytes += total;
            return total;
        }
    }

    this->decoded_bytes += total;
    return total;
}

void StreamDecoder::close_slot(uint8_t smart_toggle)
{
    this->decoded_bytes = 0;
    this->position = 0;

    if (smart_toggle == 0) {
        this->active_file = 1;
        if (this->memory_files[1].data != 0) {
            sound_stream_decoder_clear_ogg_vorbis_file(this->ogg_vorbis_file[1]);
            this->memory_files[1].position = 0;
            this->memory_files[1].data = nullptr;
            this->memory_files[1].size = 0;
            this->memory_files[1].end_of_file = 0;
        }
    } else {
        if (this->memory_files[0].data == 0) {
            if (this->memory_files[1].data == 0) {
                this->active_file = this->active_file == 0;
            } else {
                this->active_file = 1;
            }
        } else if (this->memory_files[1].data != 0) {
            this->active_file = this->active_file == 0;
        } else {
            this->active_file = 0;
        }

        if (this->active_file == 0) {
            sound_stream_decoder_clear_ogg_vorbis_file(this->ogg_vorbis_file[1]);
            this->memory_files[1].position = 0;
            this->memory_files[1].data = nullptr;
            this->memory_files[1].size = 0;
            this->memory_files[1].end_of_file = 0;
            this->open = 0;
            return;
        }
    }

    sound_stream_decoder_clear_ogg_vorbis_file(this->ogg_vorbis_file[0]);
    this->memory_files[0].position = 0;
    this->memory_files[0].data = nullptr;
    this->memory_files[0].size = 0;
    this->memory_files[0].end_of_file = 0;
    this->open = 0;
}

uint32_t StreamDecoder::fill_buffer(SoundPermutation *permutation, void *destination, uint32_t requested_size, char *want_crosslap, uint32_t *bytes_filled_out)
{
    uint32_t sample_pointer = *(uint32_t *)&((struct SoundPermutation *)permutation)->cache_page;
    int32_t remaining;

    if (sample_pointer < (uint32_t)halo::cache::globals().sound_cache_memory ||
        (uint32_t)sound_cache_size_megabytes * 0x100000 + (uint32_t)halo::cache::globals().sound_cache_memory <
            permutation->samples.size + sample_pointer) {
        if (strlen(error_text_buffer) < 0xd6) {
            sprintf(error_text_buffer + strlen(error_text_buffer), "trying to queue sound but samples is null.");
        }
    } else {
        uint32_t bytes_read;

        if (this->open == 0) {
            view(this)->open_stream((void *)sample_pointer, permutation->samples.size);
        } else {
            *want_crosslap = 0;
        }

        bytes_read = (uint32_t)view(this)->read_stream((char *)destination, requested_size, want_crosslap);
        *bytes_filled_out = bytes_read;

        if (bytes_read == 0 && permutation->buffer_size != (uint32_t)this->decoded_bytes) {
            uint32_t pad = permutation->buffer_size - this->decoded_bytes;

            if (requested_size <= pad) {
                pad = requested_size;
            }
            *bytes_filled_out = pad;
            this->decoded_bytes += pad;

            memset(destination, 0, pad);
            sound_ogg_underrun_count += 1;
        }
    }

    remaining = (int32_t)(permutation->buffer_size - this->decoded_bytes);
    return (remaining < 1) ? 0 : (uint32_t)remaining;
}


}  // namespace halo::sound
