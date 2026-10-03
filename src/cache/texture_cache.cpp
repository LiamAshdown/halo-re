#include "tags.h"

#include "halo/cache/cache.hpp"

#include "memory.h"
#include "math.h"
#include "win32.h"
#include "halo/cache/globals.hpp"
#include "halo/sound/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/main/api.hpp"

extern "C" {
typedef int32_t (__stdcall *d3d_release_fn)(void *object);
extern uint8_t debug_texture_cache_prints;
extern uint8_t rasterizer_bitmap_create_hardware_texture(BitmapData *bitmap);
extern void rasterizer_bitmap_upload_2d_mipmaps(BitmapData *bitmap);
extern void rasterizer_bitmap_upload_cubemap_mipmaps_by_face(BitmapData *bitmap);
extern void rasterizer_bitmap_upload_cubemap_mipmaps(BitmapData *bitmap);
extern void *rasterizer_get_capture_surface(uint8_t *object, void *fallback);
extern void *texture_cache_base;
extern uint32_t bitmap_compute_texture_data_size(BitmapData *bitmap);
}

namespace halo::cache {

/**
 * Cache in-use predicate for texture pages: nonzero while the entry's read has not finished.
 *
 * @address 0x444700
 */
uint8_t texture_cache_manager::entry_in_use(datum_index handle)
{
    uint8_t *entry = (uint8_t *)globals().texture_cache_entries->data + (handle & 0xffff) * 0x10;
    return entry[4] == 0;
}

/**
 * Cache release procedure: releases the entry's Direct3D texture and clears the owning bitmap's
 * references.
 *
 * @address 0x444730
 */
void texture_cache_manager::entry_release(datum_index handle)
{
    texture_cache_entry *entry = (texture_cache_entry *)globals().texture_cache_entries->data + (handle & 0xffff);
    BitmapData *bitmap;
    void *texture;

    while (((texture_cache_entry *)globals().texture_cache_entries->data + (handle & 0xffff))->loaded == 0) {
        Sleep(0);
    }
    bitmap = entry->bitmap;
    bitmap->pointer = (uint32_t)-1;
    if (bitmap->pixel_base != 0) {
        GlobalFree(bitmap->pixel_base);
        bitmap->pixel_base = 0;
    }
    bitmap = entry->bitmap;
    if ((*(uint8_t *)&bitmap->flags & 0x80) != 0) {
        if ((int32_t)bitmap->pointer != -1) {
            halo::memory::view(globals().texture_cache)->evict_entry((datum_index)bitmap->pointer);
        }
        bitmap->pointer = (uint32_t)-1;
        bitmap->pixel_base = 0;
    }
    texture = (void *)bitmap->hardware_texture;
    if (texture != 0) {
        ((d3d_release_fn)(*(void ***)texture)[2])(texture);
        bitmap->hardware_texture = 0;
    }
    halo::memory::view(globals().texture_cache_entries)->delete_datum(handle);
}

/**
 * Returns the Direct3D texture of a bitmap. Bitmaps that are not streamed return the texture stored on
 * the tag; streamed ones get a page allocated on demand, their LRU age touched and, when wait is set,
 * the read raised in priority and awaited, converting the pixels on first use.
 *
 * Returns the address of the entry's texture field, or 0 while the page is not resident.
 *
 * @address 0x444550
 */
void *texture_cache_manager::get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing)
{
    texture_cache_entry *entry;
    void *result;
    large_integer counter;
    int32_t elapsed_ms;
    int16_t bitmap_type;

    if ((bitmap->flags & 0x80) != 0) {
        if (bitmap->pointer == 0xffffffff && allocate_if_missing != 0) {
            halo::cache::texture_cache_manager::page_allocate(bitmap, wait);
        }

        if (bitmap->pointer != 0xffffffff) {
            entry = (texture_cache_entry *)((uint8_t *)globals().texture_cache_entries->data +
                (bitmap->pointer & 0xffff) * sizeof(texture_cache_entry));

            ((cache_entry *)((uint8_t *)globals().texture_cache->entries->data +
                (bitmap->pointer & 0xffff) * sizeof(cache_entry)))->age = globals().texture_cache->age;

            if (wait != 0 && entry->loaded == 0) {
                if (debug_texture_cache_prints != 0) {

                    halo::main::console_print_va("%s",
                        globals().tag_instances[(int16_t)bitmap->bitmap_tag_id.index].path);
                }
                globals().cache_io_requests[entry->io_request_index].priority = 1;
            }

            for (;;) {
                if (entry->loaded != 0) {
                    if (entry->converted == 0) {
                        entry->converted = 1;
                        rasterizer_bitmap_create_hardware_texture(bitmap);
                        bitmap_type = bitmap->type;
                        if (bitmap_type == 0) {
                            rasterizer_bitmap_upload_2d_mipmaps(bitmap);
                        } else if (bitmap_type == 1) {
                            rasterizer_bitmap_upload_cubemap_mipmaps(bitmap);
                        } else if (bitmap_type == 2) {
                            rasterizer_bitmap_upload_cubemap_mipmaps_by_face(bitmap);
                        }
                        if (bitmap->pixel_base != (void *)0) {
                            GlobalFree(bitmap->pixel_base);
                            bitmap->pixel_base = (void *)0;
                        }
                        entry->texture = *(void **)&bitmap->hardware_texture;
                    }
                    result = &entry->texture;
                    break;
                }

                QueryPerformanceCounter((LARGE_INTEGER *)&counter);
                elapsed_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
                if (0x84 < (uint32_t)(elapsed_ms - halo::sound::globals().time)) {
                    halo::sound::sound_idle_update();
                }
                if (wait == 0) {
                    return (void *)0;
                }
                Sleep(0);
            }
        } else {
            result = (void *)0;
        }
    } else {
        result = *(void **)&bitmap->hardware_texture;
    }

    if (wait != 0 && result == (void *)0) {

        result = rasterizer_get_capture_surface((uint8_t *)bitmap, (void *)0);
        return result;
    }
    return result;
}

/**
 * Creates the texture cache: the "pc texture" data_array of 0x1000 entries and a cache container over
 * texture_cache_memory that hands out slot handles, not pixel storage.
 *
 * @address 0x4444d0
 */
void texture_cache_manager::initialize()
{
    void *cache_memory;

    globals().texture_cache_entries = halo::memory::data_array_view::create(sizeof(texture_cache_entry), (char *)"pc texture", k_texture_cache_maximum_entries);

    cache_memory = GlobalAlloc(0, 0x1c07c);
    if (cache_memory != (void *)0) {
        halo::memory::view((struct cache *)cache_memory)->initialize((char *)"pc texture cache", k_texture_cache_maximum_entries, k_texture_cache_block_shift, k_texture_cache_maximum_entries, (void *)&texture_cache_manager::entry_release, (void *)&texture_cache_manager::entry_in_use);
    }
    globals().texture_cache = (struct cache *)cache_memory;
    texture_cache_base = globals().texture_cache_memory;
    return;
}

/**
 * Allocates a texture cache slot for a bitmap and submits the asynchronous read that fills a newly
 * allocated staging buffer with its pixels. Carries over the bitmap's existing texture pointer.
 * Returns 1 on success, 0 when the cache has no free slot.
 *
 * @address 0x444800
 */
uint32_t texture_cache_manager::page_allocate(BitmapData *bitmap, uint8_t priority)
{
    uint32_t computed_size;
    datum_index cache_slot;
    uint32_t alloc_size;
    void *staging_buffer;
    texture_cache_entry *entry;
    uint8_t data_file_index;
    cache_io_completion completion;

    computed_size = bitmap_compute_texture_data_size(bitmap);
    cache_slot = halo::memory::view(globals().texture_cache)->allocate_block(4);
    if (cache_slot == (datum_index)0xffffffff) {
        return 0;
    }

    alloc_size = bitmap->pixel_data_size;
    if ((int32_t)alloc_size < (int32_t)computed_size) {
        alloc_size = computed_size;
    }
    staging_buffer = GlobalAlloc(0, alloc_size);

    halo::memory::view(globals().texture_cache_entries)->new_at_index_with_salt(cache_slot);
    entry = (texture_cache_entry *)((uint8_t *)globals().texture_cache_entries->data +
        (cache_slot & 0xffff) * sizeof(texture_cache_entry));

    bitmap->pointer = cache_slot;
    bitmap->pixel_base = staging_buffer;
    entry->bitmap = bitmap;
    entry->texture = *(void **)&bitmap->hardware_texture;

    data_file_index = (bitmap->flags & 0x100) ? (uint8_t)_cache_io_data_file_bitmaps
                                               : (uint8_t)_cache_io_data_file_cache;
    completion.flag = &entry->loaded;
    completion.procedure = 0;
    completion.data = 0;
    entry->io_request_index = halo::cache::cache_io::request_new(&completion, bitmap->pixel_data_offset, bitmap->pixel_data_size, staging_buffer, priority, data_file_index);

    return 1;
}

} // namespace halo::cache
