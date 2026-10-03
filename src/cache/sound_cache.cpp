#include "tags.h"

#include "halo/cache/cache.hpp"

#include "win32.h"
#include "memory.h"
#include <stdint.h>
#include "crt.h"
#include "math.h"
#include "halo/cache/globals.hpp"
#include "halo/sound/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cseries/api.hpp"



namespace halo::cache {

/**
 * Decodes a format 1 permutation's compressed samples into the shared decode buffer, growing it as
 * needed, and copies the result into the permutation's resident page.
 *
 * @address 0x443d60
 */
void sound_cache_manager::decode_permutation(SoundPermutation *permutation)
{
    uint32_t buffer_size;
    void *decode_context;
    uint8_t *source;
    uint8_t *destination;
    uint32_t words;
    uint32_t tail_bytes;

    if (permutation->format == 1) {
        buffer_size = permutation->buffer_size;

        if (globals().sound_decode_buffer_size < (int32_t)buffer_size) {
            if (globals().sound_decode_buffer != (void *)0) {
                GlobalFree(globals().sound_decode_buffer);
            }
            globals().sound_decode_buffer_size = buffer_size;
            globals().sound_decode_buffer = GlobalAlloc(0, buffer_size);
        }

        {
            Sound *sound_tag = (Sound *)globals().tag_instances[*(datum_index *)&permutation->tag_id_1 & 0xffff].data;
            int16_t channel_count = (int16_t)(1 + (sound_tag->channel_count == 1));
            decode_context = permutation->cache_page;
            if (halo::sound::sound_decode_dispatch(channel_count, globals().sound_decode_buffer, decode_context,
                                      (int32_t)permutation->samples.size) != 0) {
                return;
            }
        }
        {
            source = (uint8_t *)globals().sound_decode_buffer;
            destination = (uint8_t *)permutation->cache_page;

            for (words = buffer_size >> 2; words != 0; words--) {
                *(uint32_t *)destination = *(uint32_t *)source;
                source += 4;
                destination += 4;
            }
            for (tail_bytes = buffer_size & 3; tail_bytes != 0; tail_bytes--) {
                *destination = *source;
                source += 1;
                destination += 1;
            }
        }
    }
    return;
}

/**
 * Tears the sound cache down: releases every live entry's page through its permutation, invalidates
 * the entry array and frees the decode buffer.
 *
 * @address 0x443f30
 */
void sound_cache_manager::dispose()
{
    data_iterator iterator;
    sound_cache_entry *entry;

    iterator.data = globals().sound_cache_entries;
    iterator.next_index = 0;
    iterator.index = 0;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    entry = (sound_cache_entry *)halo::memory::view(&iterator)->next();
    while (entry != (sound_cache_entry *)0) {
        halo::cache::sound_cache_manager::release_page(entry->permutation);
        entry = (sound_cache_entry *)halo::memory::view(&iterator)->next();
    }

    globals().sound_cache_entries->valid = 0;

    if (globals().sound_decode_buffer != (void *)0) {
        GlobalFree(globals().sound_decode_buffer);
        globals().sound_decode_buffer = (void *)0;
        globals().sound_decode_buffer_size = 0;
    }
    return;
}

/**
 * Writes the sound cache page statistics and one line per live entry to sound_cache_dump.txt.
 *
 * @address 0x444240
 */
void sound_cache_manager::dump_to_file()
{
    int32_t saved_page_count;
    int32_t allocated_pages;
    int32_t current_pages;
    int32_t old_pages;
    int32_t locked_pages;
    int32_t sound_count;
    uint8_t *bitmap;
    void *file;
    char line[1028];
    char *scan;
    int32_t bit;
    uint32_t page;
    float total_mb;
    float free_pages;
    data_iterator iterator;
    sound_cache_entry *entry;
    SoundPermutation *permutation;
    int32_t entry_number;

    saved_page_count = globals().sound_cache_page_count;
    current_pages = 0;
    allocated_pages = 0;
    old_pages = 0;
    locked_pages = 0;
    sound_count = 0;

    bitmap = (uint8_t *)GlobalAlloc(0, globals().sound_cache_page_count);
    file = fopen("sound_cache_dump.txt", globals().file_open_mode_w);

    for (scan = line, bit = 0x100; bit != 0; bit--) {
        scan[0] = 0; scan[1] = 0; scan[2] = 0; scan[3] = 0;
        scan += 4;
    }

    if (file != (void *)0) {
        halo::memory::view(globals().sound_cache)->build_status_bitmap(bitmap);

        for (bit = 0; bit < 4; bit++) {
            for (page = 0; page < (uint32_t)globals().sound_cache_page_count; page++) {
                if ((bitmap[page] & (1 << (bit & 0x1f))) != 0) {
                    if (bit == 0) {
                        allocated_pages++;
                    } else if (bit == 1) {
                        current_pages++;
                    } else if (bit == 2) {
                        old_pages++;
                    } else {
                        locked_pages++;
                    }
                }
            }
        }

        iterator.data = globals().sound_cache_entries;
        iterator.next_index = 0;
        iterator.index = 0;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        entry = (sound_cache_entry *)halo::memory::view(&iterator)->next();
        while (entry != (sound_cache_entry *)0) {
            sound_count++;
            entry = (sound_cache_entry *)halo::memory::view(&iterator)->next();
        }

        {
            float mb_total = (float)globals().sound_cache_size_megabytes;
            float page_count_f = (float)saved_page_count;
            if (saved_page_count < 0) {
                page_count_f = page_count_f + 4.2949673e+09f;
            }
            free_pages = (mb_total / page_count_f) * (page_count_f - (float)allocated_pages);

            sprintf(line,
                "%d / 512 sounds in cache\n%.2f MB / %.2f MB used %.2f percent free\n%d / %d pages allocated\n%d / %d pages used this frame\n%d / %d pages old\n%d / %d pages locked\n\n",
                sound_count, (double)(mb_total - free_pages), (double)(int32_t)globals().sound_cache_size_megabytes,
                (double)((free_pages / mb_total) * 100.0f),
                allocated_pages, saved_page_count, current_pages, saved_page_count,
                old_pages, saved_page_count, locked_pages, saved_page_count);
        }

        for (scan = line; *scan != '\0'; scan++) {
        }
        fwrite(line, 1, (uint32_t)(scan - (line + 1)), (FILE *)file);

        entry_number = 1;
        for (scan = line, bit = 0x100; bit != 0; bit--) {
            scan[0] = 0; scan[1] = 0; scan[2] = 0; scan[3] = 0;
            scan += 4;
        }
        fwrite("[sounds in cache]\n\n", 1, 0x12, (FILE *)file);

        iterator.data = globals().sound_cache_entries;
        iterator.next_index = 0;
        iterator.index = 0;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        entry = (sound_cache_entry *)halo::memory::view(&iterator)->next();
        while (entry != (sound_cache_entry *)0) {
            permutation = entry->permutation;
            if (permutation != (SoundPermutation *)0) {
                sprintf(line, "%d - %s %d c bytes %d u bytes\n", entry_number,
                    globals().tag_instances[permutation->tag_id_1.index].path,
                    permutation->samples.size, permutation->buffer_size);
                for (scan = line; *scan != '\0'; scan++) {
                }
                fwrite(line, 1, (uint32_t)(scan - (line + 1)), (FILE *)file);
                entry_number++;
            }
            entry = (sound_cache_entry *)halo::memory::view(&iterator)->next();
        }

        fclose((FILE *)file);
    }

    GlobalFree(bitmap);
    return;
}

/**
 * Cache in-use predicate: nonzero while the entry's page is locked or playing, which protects it from
 * eviction.
 *
 * @address 0x444060
 */
uint8_t sound_cache_manager::entry_in_use(datum_index handle)
{
    uint8_t *entry = (uint8_t *)globals().sound_cache_entries->data + (handle & 0xffff) * 0x10;
    return entry[2] == 0 || entry[5] != 0 || entry[6] != 0;
}

/**
 * Cache release procedure: clears the owning permutation's page references before the entry is
 * evicted.
 *
 * @address 0x4440a0
 */
void sound_cache_manager::entry_release(datum_index handle)
{
    sound_cache_entry *entry = (sound_cache_entry *)globals().sound_cache_entries->data + (handle & 0xffff);
    SoundPermutation *permutation = entry->permutation;

    permutation->samples_pointer = (uint32_t)-1;
    permutation->cache_page = 0;
    halo::memory::view(globals().sound_cache_entries)->delete_datum(handle);
}

/**
 * Creates the sound cache: the "pc sound" data_array of 0x200 entries and the 4096 byte page cache
 * over sound_cache_memory, sized from sound_cache_size_megabytes.
 *
 * @address 0x443ca0
 */
void sound_cache_manager::initialize()
{
    int32_t scaled_megabytes;
    void *cache_memory;

    globals().sound_cache_entries = halo::memory::data_array_view::create(sizeof(sound_cache_entry), (char *)"pc sound", k_sound_cache_maximum_entries);

    scaled_megabytes = (int32_t)*(int16_t *)&globals().sound_cache_size_megabytes * 0x100000;
    globals().sound_cache_page_count = (scaled_megabytes + ((scaled_megabytes >> 0x1f) & 0xfff)) >> k_sound_cache_page_shift;

    cache_memory = GlobalAlloc(0, 0x387c);
    if (cache_memory != (void *)0) {
        halo::memory::view((struct cache *)cache_memory)->initialize((char *)"pc sound cache", globals().sound_cache_page_count, k_sound_cache_page_shift, k_sound_cache_maximum_entries, (void *)&sound_cache_manager::entry_release, (void *)&sound_cache_manager::entry_in_use);
    }
    globals().sound_cache = (struct cache *)cache_memory;
    globals().sound_cache_base = globals().sound_cache_memory;
    globals().sound_cache_initialized = 1;
    return;
}

/**
 * Allocates a cache page for a permutation's samples and submits the asynchronous read that fills it.
 * When no block is available it dumps the cache statistics and crashes through a null write, as the
 * original does.
 *
 * @address 0x4440e0
 */
void sound_cache_manager::page_allocate(SoundPermutation *permutation, uint8_t priority)
{
    int32_t requested_bytes;
    uint8_t data_file_index;
    datum_index page_datum;
    int32_t page_address;
    sound_cache_entry *entry;
    cache_io_completion completion;

    requested_bytes = 0;
    data_file_index = 0;
    if ((permutation->samples.flags & 1) != 0) {
        data_file_index = 2;
    }

    if (permutation->format == 1) {
        int32_t channel_factor;
        Sound *sound;

        sound = (Sound *)globals().tag_instances[permutation->tag_id_1.index].data;
        channel_factor = (sound->channel_count == 1) + 1;
        requested_bytes =
            (channel_factor * 0x400 >> 3) *
            ((int32_t)permutation->samples.size /
            (int32_t)((channel_factor + (uint32_t)((uint16_t)(((uint16_t)((int16_t)(channel_factor * 0xfc) + 7U) >> 3) + 7) >> 3) *
                           2) * 4));
        permutation->buffer_size = requested_bytes;
    } else if (permutation->format == 3 || permutation->format == 0) {
        requested_bytes = permutation->samples.size;
    }

    page_datum = halo::memory::view(globals().sound_cache)->allocate_block((uint32_t)requested_bytes);
    if (page_datum != 0xffffffff) {
        page_address = (((cache_entry *)((uint8_t *)globals().sound_cache->entries->data +
            (page_datum & 0xffff) * sizeof(cache_entry)))->offset << (globals().sound_cache->block_shift & 0x1f)) +
            (int32_t)globals().sound_cache_base;

        halo::memory::view(globals().sound_cache_entries)->new_at_index_with_salt(page_datum);
        entry = (sound_cache_entry *)((uint8_t *)globals().sound_cache_entries->data + (page_datum & 0xffff) * sizeof(sound_cache_entry));

        permutation->samples_pointer = page_datum;
        permutation->cache_page = (void *)page_address;
        entry->permutation = permutation;

        completion.flag = &entry->loaded;
        completion.procedure = (permutation->format != 1) ? &cache_io::sound_decode_thunk : 0;
        completion.data = entry;
        entry->io_request_index = halo::cache::cache_io::request_new(&completion, permutation->samples.file_offset, permutation->samples.size, (void *)page_address, priority, data_file_index);
        return;
    }

    halo::cache::sound_cache_manager::dump_to_file();
    *(uint8_t *)0 = 1;
    return;
}

/**
 * Releases the page of every sound entry that is neither locked nor playing. Does nothing before the
 * cache exists.
 *
 * @address 0x443fd0
 */
void sound_cache_manager::release_unused()
{
    data_iterator iterator;
    sound_cache_entry *entry;

    if (globals().sound_cache_entries != (data_array *)0 && globals().sound_cache_entries->valid != 0) {
        iterator.data = globals().sound_cache_entries;
        iterator.next_index = 0;
        iterator.index = 0;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        entry = (sound_cache_entry *)halo::memory::view(&iterator)->next();
        while (entry != (sound_cache_entry *)0) {
            if (entry->lock_count == 0 && entry->playing == 0) {
                halo::cache::sound_cache_manager::release_page(entry->permutation);
            }
            entry = (sound_cache_entry *)halo::memory::view(&iterator)->next();
        }
    }
    return;
}

/**
 * Makes a permutation's sample page resident: allocates one when missing and allowed, raises the read
 * priority and waits for the load when asked, decodes on first use and optionally pins the entry.
 * Returns 1 on success; only the low byte of the result is meaningful.
 *
 * @address 0x443e10
 */
uint8_t sound_cache_manager::touch(uint8_t allocate_if_missing, uint8_t lock, uint8_t wait_until_loaded, SoundPermutation *permutation)
{
    sound_cache_entry *entry;
    large_integer counter;
    int32_t elapsed_ms;
    uint32_t stall_ms;

    if (permutation->samples_pointer == 0xffffffff) {
        if (allocate_if_missing != 0) {

            halo::cache::sound_cache_manager::page_allocate(permutation, wait_until_loaded);
        }
        if (permutation->samples_pointer == 0xffffffff) {
            return 0;
        }
    }

    entry = (sound_cache_entry *)((uint8_t *)globals().sound_cache_entries->data +
        (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));

    ((cache_entry *)((uint8_t *)globals().sound_cache->entries->data +
        (permutation->samples_pointer & 0xffff) * sizeof(cache_entry)))->age = globals().sound_cache->age;

    if (wait_until_loaded != 0 && entry->loaded == 0) {
        globals().cache_io_requests[entry->io_request_index].priority = 1;
    }

    for (;;) {
        if (entry->loaded != 0) {
            if (entry->decoded == 0) {
                entry->decoded = 1;
                entry->lock_count = 0;
                entry->playing = 0;
                halo::cache::sound_cache_manager::decode_permutation(permutation);
            }
            if (lock != 0) {
                entry->lock_count = entry->lock_count + 1;
            }
            return 1;
        }

        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        elapsed_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
        stall_ms = (uint32_t)(elapsed_ms - halo::sound::globals().time);
        if (0x84 < stall_ms) {
            halo::sound::sound_idle_update();
        }

        if (wait_until_loaded == 0) {
            break;
        }
        Sleep(0);
    }

    return 0;
}

/**
 * Releases a permutation's page cache reference if it holds one and resets its handle to -1.
 *
 * @address 0x443d30
 */
void sound_cache_manager::release_page(SoundPermutation *permutation)
{
    if (permutation->samples_pointer != 0xffffffff) {
        halo::memory::view(globals().sound_cache)->evict_entry((datum_index)permutation->samples_pointer);
    }
    permutation->samples_pointer = 0xffffffff;
    permutation->cache_page = 0;
    return;
}

/**
 * Touches every live permutation of every pitch range of a sound tag into the cache without locking or
 * waiting.
 *
 * @address 0x444a60
 */
void sound_cache_manager::touch_tag_permutations(TagID tag)
{
    Sound *sound;
    int32_t pitch_range_index;
    SoundPitchRange *pitch_range;
    int16_t permutation_index;
    SoundPermutation *permutations;

    sound = (Sound *)globals().tag_instances[tag.index].data;

    if (0 < (int32_t)sound->pitch_ranges.count) {
        pitch_range_index = 0;
        do {
            pitch_range = &((SoundPitchRange *)sound->pitch_ranges.pointer)[pitch_range_index];
            permutations = (SoundPermutation *)pitch_range->permutations.pointer;

            if (0 < pitch_range->actual_permutation_count) {
                permutation_index = 0;
                do {
                    halo::cache::sound_cache_manager::touch(1, 0, 0, &permutations[permutation_index]);
                    permutation_index = permutation_index + 1;
                } while (permutation_index < (int16_t)pitch_range->actual_permutation_count);
            }

            pitch_range_index = pitch_range_index + 1;
        } while (pitch_range_index < (int32_t)sound->pitch_ranges.count);
    }
    return;
}

} // namespace halo::cache
