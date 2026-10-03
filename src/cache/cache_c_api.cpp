#include "tags.h"

#include "halo/cache/cache.hpp"

extern "C" {

datum_index tag_iterator_next(tag_iterator *iterator)
{
    return halo::cache::view(iterator)->next();
}

datum_index tag_lookup(tag_group group, char *path)
{
    return halo::cache::tag_table::lookup(group, path);
}

void cache_file_download_finish(void)
{
    halo::cache::cache_files::download_finish();
}

uint8_t cache_file_download_matches(char *name)
{
    return halo::cache::cache_files::download_matches(name);
}

int16_t cache_file_download_poll(float *progress_out)
{
    return halo::cache::cache_files::download_poll(progress_out);
}

int16_t cache_file_download_status_get(float *progress_out, int32_t unaff_ecx)
{
    return halo::cache::cache_files::download_status_get(progress_out, unaff_ecx);
}

void cache_file_download_stop(void)
{
    halo::cache::cache_files::download_stop();
}

uint8_t cache_file_exists(char *name, cache_file_header *header_out)
{
    return halo::cache::cache_files::exists(name, header_out);
}

int16_t cache_file_find_oldest_slot(cache_file_slot_category slot_category, int32_t required_size)
{
    return halo::cache::cache_files::find_oldest_slot(slot_category, required_size);
}

int16_t cache_file_find_slot_by_name(char *filename)
{
    return halo::cache::cache_files::find_slot_by_name(filename);
}

datum_index cache_file_load(char *path)
{
    return halo::cache::cache_files::load(path);
}

uint8_t cache_file_open_by_name(char *name, uint8_t report_fatal_error)
{
    return halo::cache::cache_files::open_by_name(name, report_fatal_error);
}

uint8_t cache_file_request_map(char *name, uint8_t quit_on_fail)
{
    return halo::cache::cache_files::request_map(name, quit_on_fail);
}

void cache_file_slot_read_header(int32_t slot_index)
{
    halo::cache::cache_files::slot_read_header(slot_index);
}

void cache_file_unload(void)
{
    halo::cache::cache_files::unload();
}

void cache_reserve_map_memory(void)
{
    halo::cache::cache_files::reserve_map_memory();
}

void cache_io_completion_routine(uint32_t error_code, uint32_t bytes_transferred, cache_io_request *overlapped)
{
    halo::cache::cache_io::completion_routine(error_code, bytes_transferred, overlapped);
}

void cache_io_read_file_ex_retry(void *read_file_ex, void *file, void *buffer, cache_io_request *request, uint32_t size, uint32_t offset, void *completion_routine)
{
    halo::cache::cache_io::read_file_ex_retry(read_file_ex, file, buffer, request, size, offset, completion_routine);
}

void __stdcall cache_io_request_completion_routine(uint32_t error_code, uint32_t bytes_transferred, cache_io_request *overlapped)
{
    return halo::cache::cache_io::request_completion_routine(error_code, bytes_transferred, overlapped);
}

int16_t cache_io_request_find_free_slot(void)
{
    return halo::cache::cache_io::request_find_free_slot();
}

int16_t cache_io_request_new(cache_io_completion *completion, int32_t offset, uint32_t size, void *destination, uint8_t priority, uint8_t data_file_index)
{
    return halo::cache::cache_io::request_new(completion, offset, size, destination, priority, data_file_index);
}

void cache_io_sound_decode_thunk(cache_io_completion *record)
{
    halo::cache::cache_io::sound_decode_thunk(record);
}

uint32_t cache_io_thread_proc_async(void *parameter)
{
    return halo::cache::cache_io::thread_proc_async(parameter);
}

uint32_t cache_io_thread_proc_sync(void *parameter)
{
    return halo::cache::cache_io::thread_proc_sync(parameter);
}

void cache_io_thread_start(void)
{
    halo::cache::cache_io::thread_start();
}

void cache_io_wait_all_requests(void)
{
    halo::cache::cache_io::wait_all_requests();
}

uint8_t cache_io_wait_for_flag(uint8_t *flag)
{
    return halo::cache::cache_io::wait_for_flag(flag);
}

uint8_t data_file_read_data_block(data_file *file)
{
    return halo::cache::view(file)->read_data_block();
}

int32_t data_file_read_header(data_file *file, int32_t expected_file_id)
{
    return halo::cache::view(file)->read_header(expected_file_id);
}

uint8_t data_file_read_offset_table(data_file *file)
{
    return halo::cache::view(file)->read_offset_table();
}

void data_file_open(void)
{
    halo::cache::data_files::open();
}

void data_file_close(void)
{
    halo::cache::data_files::close();
}

void sound_cache_decode_permutation(SoundPermutation *permutation)
{
    halo::cache::sound_cache_manager::decode_permutation(permutation);
}

void sound_cache_dispose(void)
{
    halo::cache::sound_cache_manager::dispose();
}

void sound_cache_dump_to_file(void)
{
    halo::cache::sound_cache_manager::dump_to_file();
}

uint8_t sound_cache_entry_in_use(datum_index handle)
{
    return halo::cache::sound_cache_manager::entry_in_use(handle);
}

void sound_cache_entry_release(datum_index handle)
{
    halo::cache::sound_cache_manager::entry_release(handle);
}

void sound_cache_new(void)
{
    halo::cache::sound_cache_manager::initialize();
}

void sound_cache_page_allocate(SoundPermutation *permutation, uint8_t priority)
{
    halo::cache::sound_cache_manager::page_allocate(permutation, priority);
}

void sound_cache_release_unused(void)
{
    halo::cache::sound_cache_manager::release_unused();
}

uint8_t sound_cache_touch(uint8_t allocate_if_missing, uint8_t lock, uint8_t wait_until_loaded, SoundPermutation *permutation)
{
    return halo::cache::sound_cache_manager::touch(allocate_if_missing, lock, wait_until_loaded, permutation);
}

void sound_permutation_release_page(SoundPermutation *permutation)
{
    halo::cache::sound_cache_manager::release_page(permutation);
}

void sound_tag_touch_permutations(TagID tag)
{
    halo::cache::sound_cache_manager::touch_tag_permutations(tag);
}

uint8_t texture_cache_entry_in_use(datum_index handle)
{
    return halo::cache::texture_cache_manager::entry_in_use(handle);
}

void texture_cache_entry_release(datum_index handle)
{
    halo::cache::texture_cache_manager::entry_release(handle);
}

void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing)
{
    return halo::cache::texture_cache_manager::get(bitmap, wait, allocate_if_missing);
}

void texture_cache_new(void)
{
    halo::cache::texture_cache_manager::initialize();
}

uint32_t texture_cache_page_allocate(BitmapData *bitmap, uint8_t priority)
{
    return halo::cache::texture_cache_manager::page_allocate(bitmap, priority);
}

void structure_bsp_dispose(ScenarioBSP *bsp)
{
    halo::cache::structure_bsp_loader::dispose(bsp);
}

void structure_bsp_dispose_material_vertex_buffers(ScenarioStructureBSPCompiledHeader *compiled_header)
{
    halo::cache::structure_bsp_loader::dispose_material_vertex_buffers(compiled_header);
}

uint32_t structure_bsp_load(ScenarioBSP *bsp)
{
    return halo::cache::structure_bsp_loader::load(bsp);
}

void structure_bsp_load_material_vertex_buffers(ScenarioStructureBSPCompiledHeader *compiled_header)
{
    halo::cache::structure_bsp_loader::load_material_vertex_buffers(compiled_header);
}

void model_dispose_vertex_buffers(void)
{
    halo::cache::model_vertex_buffers::dispose();
}

void model_load_vertex_buffers(cache_file_tag_header *header)
{
    halo::cache::model_vertex_buffers::load(header);
}

void predicted_resource_list_touch(TagReflexive *resources)
{
    halo::cache::predicted_resources::touch(resources);
}

} // extern "C"
