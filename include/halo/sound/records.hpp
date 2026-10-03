/**
 * @file include/halo/sound/records.hpp
 * Member-function views of the sound module's record types. Each view derives from the C record without adding
 * data, so it has the record's exact layout; `view()` reinterprets a record pointer as its view.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound {

/**
 * Operations on a sound_location: audibility and obstruction against the listener.
 */
struct Location : sound_location {
    /**
     * Defaults a source's obstruction/occlusion to 0.6/1.0. When the source and listener clusters are within
     * sound range, a flagged cluster pair is raycast (obstruction 0.45, or 0/0 on a clear line); unless the ray
     * was clear, occlusion becomes 1.4 * (1 - reference / (distance + reference)) clamped to [0, 1].
     *
     * @address 0x00544aa0
     */
    void compute_obstruction_occlusion(int16_t listener_index, float reference_distance);

    /**
     * Checks whether `location` is close enough to be audible (within max_distance of a listener, and not fully
     * occluded), and picks which listener it is closest to. Returns 0 (listener 0) if audible, or -1 if
     * `location` is unspatialized, too far, or fully occluded (occlusion == 1.0).
     *
     * @address 0x0054bb20
     */
    int16_t check_audibility(float max_distance);
};

static_assert(sizeof(Location) == sizeof(sound_location), "Location adds no data");

/** Returns the view of a sound_location record. */
inline Location *view(sound_location *record)
{
    return reinterpret_cast<Location *>(record);
}

/**
 * The in-memory data source of one Ogg Vorbis stream slot.
 */
struct OggMemoryFile : sound_ogg_memory_file {
    /**
     * Seek callback of the in-memory Vorbis source: positions the stream by SEEK_SET, SEEK_CUR or SEEK_END
     * semantics and clears the end-of-file flag. Returns 0 on success and -1 when the target lies outside the
     * data.
     *
     * @address 0x00544e00
     */
    int32_t seek_to(uint32_t offset_low, int32_t offset_high, int32_t whence);
};

static_assert(sizeof(OggMemoryFile) == sizeof(sound_ogg_memory_file), "OggMemoryFile adds no data");

/** Returns the view of a sound_ogg_memory_file record. */
inline OggMemoryFile *view(sound_ogg_memory_file *record)
{
    return reinterpret_cast<OggMemoryFile *>(record);
}

/**
 * Streaming Ogg Vorbis decoder with two crosslapped slots, one per channel.
 */
struct StreamDecoder : sound_stream_decoder {
    /**
     * Opens an Ogg Vorbis decode stream over an in-memory buffer, using whichever of the decoder's two
     * double-buffered slots is not currently the active one (so the playing slot survives a gapless loop
     * transition). Returns 1 on success, 0 on failure.
     *
     * @address 0x00544eb0
     */
    uint8_t open_stream(void *data, int32_t size);

    /**
     * Decodes up to `size` bytes of PCM from the decoder's currently active Ogg Vorbis file into `buffer`,
     * performing a one-time cross-lap from the other slot into this one first when `*want_crosslap` is set.
     * Returns the number of bytes actually decoded (which may be less than `size` on EOF or a read error).
     *
     * @address 0x005451d0
     */
    int32_t read_stream(char *buffer, int32_t size, char *want_crosslap);

    /**
     * Resets the decode position and closes and zeroes one of the two Ogg Vorbis slots. With `smart_toggle`
     * zero it closes slot 1 and makes slot 1 the active one; otherwise it flips the active slot toward
     * whichever slot still holds data and closes the other.
     *
     * @address 0x00545760
     */
    void close_slot(uint8_t smart_toggle);

    /**
     * Decodes up to `requested_size` bytes of PCM from `permutation`'s cached (bounds-checked) compressed
     * sample pool into `destination` via `decoder`, opening the stream on first use. Pads the buffer with
     * silence and counts an underrun when the stream runs dry before the permutation's declared buffer_size is
     * reached. Returns the bytes still owed for this buffer (never negative).
     *
     * @address 0x00545920
     */
    uint32_t fill_buffer(SoundPermutation *permutation, void *destination, uint32_t requested_size, char *want_crosslap, uint32_t *bytes_filled_out);
};

static_assert(sizeof(StreamDecoder) == sizeof(sound_stream_decoder), "StreamDecoder adds no data");

/** Returns the view of a sound_stream_decoder record. */
inline StreamDecoder *view(sound_stream_decoder *record)
{
    return reinterpret_cast<StreamDecoder *>(record);
}

}  // namespace halo::sound
