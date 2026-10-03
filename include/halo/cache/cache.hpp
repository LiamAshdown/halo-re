#pragma once

/**
 * Public API of the cache module. Include tags.h before this header: it supplies the tag structure types
 * the signatures refer to.
 */

#include "cache.h"
#include "halo/memory/memory.hpp"

#include <cstddef>
#include <type_traits>

namespace halo::cache {

/**
 * Operations on the tag_iterator record. The view adds no data members, so a tag_iterator pointer can be viewed
 * as a tag_iterator_view in place and the C layout is unchanged.
 */
struct tag_iterator_view : ::tag_iterator {
    datum_index next();
};

inline tag_iterator_view *view(::tag_iterator *record) { return static_cast<tag_iterator_view *>(record); }

/**
 * tag_table: the cache module operations of this topic, grouped as static members.
 */
struct tag_table {
    static datum_index lookup(tag_group group, char *path);
};

/**
 * cache_files: the cache module operations of this topic, grouped as static members.
 */
struct cache_files {
    static void download_finish();
    static uint8_t download_matches(char *name);
    static int16_t download_poll(float *progress_out);
    static int16_t download_status_get(float *progress_out, int32_t unaff_ecx);
    static void download_stop();
    static uint8_t exists(char *name, cache_file_header *header_out);
    static int16_t find_oldest_slot(cache_file_slot_category slot_category, int32_t required_size);
    static int16_t find_slot_by_name(char *filename);
    static datum_index load(char *path);
    static uint8_t open_by_name(char *name, uint8_t report_fatal_error);
    static uint8_t request_map(char *name, uint8_t quit_on_fail);
    static void slot_read_header(int32_t slot_index);
    static void unload();
    static void reserve_map_memory();
    static int32_t slot_size_limit(int16_t slot_index);
};

/**
 * cache_io: the cache module operations of this topic, grouped as static members.
 */
struct cache_io {
    static void completion_routine(uint32_t error_code, uint32_t bytes_transferred, cache_io_request *overlapped);
    static void read_file_ex_retry(void *read_file_ex, void *file, void *buffer, cache_io_request *request, uint32_t size, uint32_t offset, void *completion_routine);
    static void request_completion_routine(uint32_t error_code, uint32_t bytes_transferred, cache_io_request *overlapped);
    static int16_t request_find_free_slot();
    static int16_t request_new(cache_io_completion *completion, int32_t offset, uint32_t size, void *destination, uint8_t priority, uint8_t data_file_index);
    static void sound_decode_thunk(cache_io_completion *record);
    static uint32_t thread_proc_async(void *parameter);
    static uint32_t thread_proc_sync(void *parameter);
    static void thread_start();
    static void wait_all_requests();
    static uint8_t wait_for_flag(uint8_t *flag);
};

/**
 * Operations on the data_file record. The view adds no data members, so a data_file pointer can be viewed
 * as a data_file_view in place and the C layout is unchanged.
 */
struct data_file_view : ::data_file {
    uint8_t read_data_block();
    int32_t read_header(int32_t expected_file_id);
    uint8_t read_offset_table();
};

inline data_file_view *view(::data_file *record) { return static_cast<data_file_view *>(record); }

/**
 * data_files: the cache module operations of this topic, grouped as static members.
 */
struct data_files {
    static void open();
    static void close();
    static void zero(data_file *file);
};

/**
 * sound_cache_manager: the cache module operations of this topic, grouped as static members.
 */
struct sound_cache_manager {
    static void decode_permutation(SoundPermutation *permutation);
    static void dispose();
    static void dump_to_file();
    static uint8_t entry_in_use(datum_index handle);
    static void entry_release(datum_index handle);
    static void initialize();
    static void page_allocate(SoundPermutation *permutation, uint8_t priority);
    static void release_unused();
    static uint8_t touch(uint8_t allocate_if_missing, uint8_t lock, uint8_t wait_until_loaded, SoundPermutation *permutation);
    static void release_page(SoundPermutation *permutation);
    static void touch_tag_permutations(TagID tag);
};

/**
 * texture_cache_manager: the cache module operations of this topic, grouped as static members.
 */
struct texture_cache_manager {
    static uint8_t entry_in_use(datum_index handle);
    static void entry_release(datum_index handle);
    static void *get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing);
    static void initialize();
    static uint32_t page_allocate(BitmapData *bitmap, uint8_t priority);
};

/**
 * structure_bsp_loader: the cache module operations of this topic, grouped as static members.
 */
struct structure_bsp_loader {
    static void dispose(ScenarioBSP *bsp);
    static void dispose_material_vertex_buffers(ScenarioStructureBSPCompiledHeader *compiled_header);
    static uint32_t load(ScenarioBSP *bsp);
    static void load_material_vertex_buffers(ScenarioStructureBSPCompiledHeader *compiled_header);
};

/**
 * model_vertex_buffers: the cache module operations of this topic, grouped as static members.
 */
struct model_vertex_buffers {
    static void dispose();
    static void load(cache_file_tag_header *header);
};

/**
 * predicted_resources: the cache module operations of this topic, grouped as static members.
 */
struct predicted_resources {
    static void touch(TagReflexive *resources);
};

} // namespace halo::cache

