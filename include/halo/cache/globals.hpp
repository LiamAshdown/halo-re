/**
 * @file include/halo/cache/globals.hpp
 * The cache module's engine globals as one service object. The variables live at fixed addresses in the data
 * image (standalone/data) under their original link names; Globals holds a reference to each, so no other file
 * declares them. Include tags.h before this header: it supplies the tag structure types.
 */
#pragma once

#include "cache.h"
#include "memory.h"

namespace halo::cache {

/**
 * References to the cache module's engine variables.
 *
 * The block from cache_file_loaded (0x006a8150) to sound_cache_memory (0x006ac554) is the resident map: the
 * active header, the six cache file slots, the download state, the async io objects and the sound and texture
 * caches with their backing memory. tag_instances (0x0087bc14) is the tag table of the loaded map and equals
 * tag_header->tags.
 */
struct Globals {
    uint8_t &cache_file_loaded;
    cache_file_header &cache_file_current_header;
    cache_file_tag_header *&tag_header;
    void *&structure_bsp_data;
    map_download_state *&map_download;
    cache_file_slot (&cache_file_slots)[k_cache_file_slot_count];
    uint8_t &map_download_in_progress;
    int16_t &map_download_slot_index;
    char (&map_download_name)[0x20];
    int16_t &cache_file_index;
    void *&cache_io_event;
    void *&cache_io_thread;
    cache_io_request *&cache_io_requests;
    data_file &sounds_data_file;
    data_file &bitmaps_data_file;
    data_array *&sound_cache_entries;
    void *&sound_cache_base;
    ::cache *&sound_cache;
    uint8_t &sound_cache_initialized;
    data_array *&texture_cache_entries;
    ::cache *&texture_cache;
    void *&map_memory;
    void *&tag_data_base;
    void *&texture_cache_memory;
    void *&sound_cache_memory;
    int32_t &sound_cache_page_count;
    void *&sound_decode_buffer;
    int32_t &sound_decode_buffer_size;
    tag_instance *&tag_instances;
};

extern const Globals cache_globals;

/** The cache service object (single instance, constant-initialised). */
inline const Globals &globals() { return cache_globals; }

}  // namespace halo::cache
