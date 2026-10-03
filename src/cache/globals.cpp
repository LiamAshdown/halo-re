/**
 * @file src/cache/globals.cpp
 * Binds halo::cache::Globals to the engine variables the data image defines under their original link names.
 */

#include "tags.h"

#include "halo/cache/globals.hpp"

extern "C" {
extern uint8_t cache_file_loaded;
extern cache_file_header cache_file_current_header;
extern cache_file_tag_header *tag_header;
extern void *structure_bsp_data;
extern map_download_state *map_download;
extern cache_file_slot cache_file_slots[k_cache_file_slot_count];
extern uint8_t map_download_in_progress;
extern int16_t map_download_slot_index;
extern char map_download_name[0x20];
extern int16_t cache_file_index;
extern void *cache_io_event;
extern void *cache_io_thread;
extern cache_io_request *cache_io_requests;
extern data_file sounds_data_file;
extern data_file bitmaps_data_file;
extern data_array *sound_cache_entries;
extern void *sound_cache_base;
extern struct cache *sound_cache;
extern uint8_t sound_cache_initialized;
extern data_array *texture_cache_entries;
extern struct cache *texture_cache;
extern void *map_memory;
extern void *tag_data_base;
extern void *texture_cache_memory;
extern void *sound_cache_memory;
extern int32_t sound_cache_page_count;
extern void *sound_decode_buffer;
extern int32_t sound_decode_buffer_size;
extern tag_instance *tag_instances;
}

namespace halo::cache {

const Globals cache_globals{
    ::cache_file_loaded,
    ::cache_file_current_header,
    ::tag_header,
    ::structure_bsp_data,
    ::map_download,
    ::cache_file_slots,
    ::map_download_in_progress,
    ::map_download_slot_index,
    ::map_download_name,
    ::cache_file_index,
    ::cache_io_event,
    ::cache_io_thread,
    ::cache_io_requests,
    ::sounds_data_file,
    ::bitmaps_data_file,
    ::sound_cache_entries,
    ::sound_cache_base,
    ::sound_cache,
    ::sound_cache_initialized,
    ::texture_cache_entries,
    ::texture_cache,
    ::map_memory,
    ::tag_data_base,
    ::texture_cache_memory,
    ::sound_cache_memory,
    ::sound_cache_page_count,
    ::sound_decode_buffer,
    ::sound_decode_buffer_size,
    ::tag_instances,
};

}  // namespace halo::cache
