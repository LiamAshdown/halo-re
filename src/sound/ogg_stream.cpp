/**
 * @file src/sound/ogg_stream.cpp
 * Ogg Vorbis memory streams and PCM feed from the sound cache.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "internal/state.hpp"
#include "halo/cache/api.hpp"

namespace halo::sound {

namespace {

/** Clears an OggVorbis_File and zeroes its storage. */
void sound_stream_decoder_clear_ogg_vorbis_file(void *vorbis_file)
{
    uint32_t *words = (uint32_t *)vorbis_file;
    int32_t i;

    ov_clear(vorbis_file);
    for (i = 0; i < (int32_t)(k_ogg_vorbis_file_size / 4); i++) {
        words[i] = 0;
    }
}

}  // namespace

namespace stream {

uint32_t read_memory_file(void *destination, uint32_t size, uint32_t count, sound_ogg_memory_file *file)
{
    uint32_t bytes = size * count;
    uint32_t remaining;

    if (destination == 0 || file->end_of_file) {
        return 0;
    }
    remaining = (uint32_t)(file->size - file->position);
    if (bytes > remaining) {
        bytes = remaining;
        file->end_of_file = 1;
    }
    memcpy(destination, (uint8_t *)file->data + file->position, bytes);
    file->position = file->position + bytes;
    return bytes;
}

int32_t seek_memory_file(sound_ogg_memory_file *file, uint32_t offset_low, int32_t offset_high, int32_t whence)
{
    return view(file)->seek_to(offset_low, offset_high, whence);
}

int32_t close_memory_file(sound_ogg_memory_file *file)
{
    if (file == 0) {
        return -1;
    }
    file->data = 0;
    return 0;
}

int32_t tell_memory_file(sound_ogg_memory_file *file)
{
    if (file == 0) {
        return -1;
    }
    if (file->end_of_file) {
        return file->size;
    }
    return file->position;
}

void error_to_string(int32_t vorbis_error_code)
{
    char buffer[4092];

    switch (vorbis_error_code) {
    case -0x8a:
        sprintf(buffer, "The given stream is not seekable.");
        return;
    case -0x89:
        sprintf(buffer,
            "The given link exists in the Vorbis data stream, but is not decipherable due to garbacge or corruption.");
        return;
    case -0x88:
        sprintf(buffer, "Bad packet.");
        return;
    case -0x87:
        sprintf(buffer, "Not audio.");
        return;
    case -0x86:
        sprintf(buffer, "The bitstream format revision of the given stream is not supported.");
        return;
    case -0x85:
        sprintf(buffer,
            "The file/data is apparently an Ogg Vorbis stream, but contains a corrupted or undecipherable header.");
        return;
    case -0x84:
        sprintf(buffer, "The given file/data was not recognized as Ogg Vorbis data.");
        return;
    case -0x83:
        sprintf(buffer,
            "Either an invalid argument, or incompletely initialized argument passed to libvorbisfile call.");
        return;
    case -0x82:
        sprintf(buffer, "Feature not implemented.");
        return;
    case -0x81:
        sprintf(buffer, "Internal inconsistency in decode state. Continuing is likely not possible.");
        return;
    case -0x80:
        sprintf(buffer, "Read error while fetching compressed data for decode.");
        return;
    case -3:
        sprintf(buffer,
            "Vorbisfile encoutered missing or corrupt data in the bitstream. Recovery is normally automatic and this return code is for informational purposes only.");
        return;
    case -2:
        sprintf(buffer, "EOF");
        return;
    case -1:
        sprintf(buffer, "Not true, or no data available.");
        return;
    default:
        sprintf(buffer, "Unknown error");
        return;
    }
}

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

int32_t OggMemoryFile::seek_to(uint32_t offset_low, int32_t offset_high, int32_t whence)
{
    int64_t offset = ((int64_t)offset_high << 32) | offset_low;
    int64_t new_position;

    this->end_of_file = 0;

    switch (whence) {
    case 0:
        new_position = offset;
        break;
    case 1:
        new_position = (int64_t)this->position + offset;
        break;
    case 2:
        new_position = (int64_t)this->size + offset;
        break;
    default:
        return 0;
    }

    if (new_position >= 0 && new_position <= (int64_t)this->size) {
        this->position = (int32_t)new_position;
        return 0;
    }

    return -1;
}

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

    result = ov_open_callbacks(memory_file, ogg_vorbis_file, (char *)0, 0,
        (void *)stream::read_memory_file, (void *)stream::seek_memory_file, (void *)stream::close_memory_file, (void *)stream::tell_memory_file);

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
    int32_t bitstream_index;
    void *vorbis_file;
    int32_t read_result;

    for (;;) {
        if (*want_crosslap != 0 && crosslapped == 0) {
            void *current = (this->active_file == 0) ? this->ogg_vorbis_file[1] : this->ogg_vorbis_file[0];
            void *other = (this->active_file == 0) ? this->ogg_vorbis_file[0] : this->ogg_vorbis_file[1];
            int32_t crosslap_result = ov_crosslap(other, current);

            ov_clear(other);
            if (crosslap_result < 0) {
                stream::error_to_string(crosslap_result);
            }
            crosslapped = 1;
        }

        vorbis_file = (this->active_file == 0) ? this->ogg_vorbis_file[1] : this->ogg_vorbis_file[0];

        read_result = ov_read(vorbis_file, buffer + total, size, 0, 2, 1, &bitstream_index);
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

    if (read_result < 0 && read_result != -3) {
        char message[1024];
        sprintf(message, "ov_read failed trying to read %d bytes", size);
        stream::error_to_string(read_result);
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
            this->memory_files[1].data = (void *)0;
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
            this->memory_files[1].data = (void *)0;
            this->memory_files[1].size = 0;
            this->memory_files[1].end_of_file = 0;
            this->open = 0;
            return;
        }
    }

    sound_stream_decoder_clear_ogg_vorbis_file(this->ogg_vorbis_file[0]);
    this->memory_files[0].position = 0;
    this->memory_files[0].data = (void *)0;
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
