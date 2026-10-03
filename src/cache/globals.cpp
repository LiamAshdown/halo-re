/**
 * @file src/cache/globals.cpp
 * Binds halo::cache::Globals to the engine variables the data image defines under their original link names.
 */

#include "tags.h"
#include "halo/cache/cache.hpp"
#include "win32.h"
#include "crt.h"
#include "memory.h"
#include <string.h>
#include "halo/cache/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h>
#include "halo/sound/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/main/api.hpp"
#include "halo/core/crt.hpp"
#include "halo/cache/globals.hpp"
#include "link/cache.hpp"

namespace halo::cache {

const Globals &Service::instance()
{
    static const Globals state{
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
        ::map_path_prefix,
        ::os_platform,
        ::quit_confirm_error_string_index,
        ::quit_confirm_error_unknown_ae,
        ::quit_confirm_error_modal,
        ::quit_confirm_error_is_error,
        ::profile_directory,
        ::sound_cache_size_megabytes,
        ::rasterizer_vertex_sizes,
        ::file_open_mode_w,
        ::debug_texture_cache_prints,
        ::texture_cache_base,
    };
    return state;
}

}  // namespace halo::cache
