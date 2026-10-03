/**
 * @file standalone/data/link/cache.hpp
 * Link names of the engine variables the cache module binds in halo::cache::Globals (src/cache/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern char map_path_prefix[];
extern int32_t os_platform;
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern uint8_t quit_confirm_error_modal;
extern uint8_t quit_confirm_error_is_error;
extern char profile_directory[0x105];
extern int32_t sound_cache_size_megabytes;
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
extern int16_t rasterizer_vertex_sizes[];
extern char file_open_mode_w[];
extern uint8_t debug_texture_cache_prints;
extern void *texture_cache_base;
}
