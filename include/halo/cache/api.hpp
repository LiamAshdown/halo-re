/**
 * @file include/halo/cache/api.hpp
 * The public C++ API of the cache module (namespace halo::cache): the cache operations as free functions taking the
 * engine's records by pointer, in the argument order of the original functions, plus the Globals service object.
 * Include tags.h before this header: it supplies the tag structure types the signatures refer to.
 */
#pragma once

#include "halo/cache/globals.hpp"
#include "halo/cache/cache.hpp"

namespace halo::cache {

inline datum_index tag_iterator_next(tag_iterator *iterator)
{
    return halo::cache::view(iterator)->next();
}

inline datum_index tag_lookup(tag_group group, const char *path)
{
    return halo::cache::tag_table::lookup(group, path);
}

inline void cache_file_download_finish(void)
{
    halo::cache::cache_files::download_finish();
}

inline uint8_t cache_file_download_matches(char *name)
{
    return halo::cache::cache_files::download_matches(name);
}

inline int16_t cache_file_download_status_get(float *progress_out, int32_t unaff_ecx)
{
    return halo::cache::cache_files::download_status_get(progress_out, unaff_ecx);
}

inline void cache_file_download_stop(void)
{
    halo::cache::cache_files::download_stop();
}

inline uint8_t cache_file_exists(char *name, cache_file_header *header_out)
{
    return halo::cache::cache_files::exists(name, header_out);
}

inline int16_t cache_file_find_slot_by_name(char *filename)
{
    return halo::cache::cache_files::find_slot_by_name(filename);
}

inline datum_index cache_file_load(char *path)
{
    return halo::cache::cache_files::load(path);
}

inline uint8_t cache_file_open_by_name(char *name, uint8_t report_fatal_error)
{
    return halo::cache::cache_files::open_by_name(name, report_fatal_error);
}

inline uint8_t cache_file_request_map(char *name, uint8_t quit_on_fail)
{
    return halo::cache::cache_files::request_map(name, quit_on_fail);
}

inline void cache_file_unload(void)
{
    halo::cache::cache_files::unload();
}

inline void cache_reserve_map_memory(void)
{
    halo::cache::cache_files::reserve_map_memory();
}

inline void data_file_open(void)
{
    halo::cache::data_files::open();
}

inline void data_file_close(void)
{
    halo::cache::data_files::close();
}

inline void sound_cache_dump_to_file(void)
{
    halo::cache::sound_cache_manager::dump_to_file();
}

inline void sound_cache_new(void)
{
    halo::cache::sound_cache_manager::initialize();
}

inline void sound_cache_release_unused(void)
{
    halo::cache::sound_cache_manager::release_unused();
}

inline uint8_t sound_cache_touch(uint8_t allocate_if_missing, uint8_t lock, uint8_t wait_until_loaded, SoundPermutation *permutation)
{
    return halo::cache::sound_cache_manager::touch(allocate_if_missing, lock, wait_until_loaded, permutation);
}

inline void sound_permutation_release_page(SoundPermutation *permutation)
{
    halo::cache::sound_cache_manager::release_page(permutation);
}

inline void * texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing)
{
    return halo::cache::texture_cache_manager::get(bitmap, wait, allocate_if_missing);
}

inline void texture_cache_new(void)
{
    halo::cache::texture_cache_manager::initialize();
}

inline void structure_bsp_dispose(ScenarioBSP *bsp)
{
    halo::cache::structure_bsp_loader::dispose(bsp);
}

inline void structure_bsp_dispose_material_vertex_buffers(ScenarioStructureBSPCompiledHeader *compiled_header)
{
    halo::cache::structure_bsp_loader::dispose_material_vertex_buffers(compiled_header);
}

inline uint32_t structure_bsp_load(ScenarioBSP *bsp)
{
    return halo::cache::structure_bsp_loader::load(bsp);
}

inline void predicted_resource_list_touch(TagReflexive *resources)
{
    halo::cache::predicted_resources::touch(resources);
}

}  // namespace halo::cache
