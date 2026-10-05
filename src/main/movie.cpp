/**
 * Movie frame capture.
 */

#include "tags.h"
#include "halo/bitmaps/api.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "game.h"
#include "networking.h"
#include "saved_games.h"
#include "hs.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include "rasterizer.h"
#include "shell.h"
#include <stdint.h> 

#include "halo/main/movie.hpp"
#include "halo/main/layout.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/core/link.hpp"
#include "halo/main/vars.hpp"
#include "halo/main/api.hpp"

static auto &main_globals_data = halo::link::ref<main_globals>(halo::main::vars().main_globals_data);
namespace halo::main {

/**
 * Captures the current frame into movie_frame_bitmap and, once per call while capturing is
 * active, exports it as "movie\\frameNNNNNN.tga" (the frame index counting up from
 * movie_frame_index), only while no screenshot tiling is also in progress.
 *
 * @address 0x4c9530
 */
void MoviePlayer::capture_frame_export(void)
{
    char path[k_main_movie_frame_path_length];
    BitmapData *movie_frame_bitmap = (BitmapData *)main_globals_data.movie_frame_bitmap;

    halo::rasterizer::rasterizer_capture_and_present(0, movie_frame_bitmap);

    if (main_globals_data.screenshot_tile_count < 1 && movie_frame_bitmap != 0) {
        file_reference_record request;

        _snprintf(path, k_main_movie_frame_path_length, "movie\\frame%06d.tga", main_globals_data.movie_frame_index);
        main_globals_data.movie_frame_index = main_globals_data.movie_frame_index + 1;

        memset(&request, 0, sizeof(request));
        request.signature = k_file_reference_signature;
        request.location = -1;

        if ((request.flags & 1) != 0) {
            halo::saved_games::path_remove_last_component((char *)((uint8_t *)&request.path));
        }
        halo::saved_games::path_append_component(request.path, path);
        request.flags = request.flags | 1;

        halo::bitmaps::targa_export(movie_frame_bitmap, &request);
    }
}

}

static auto &movie_playback_abort = halo::link::ref<int32_t>(halo::main::vars().movie_playback_abort);
static auto &rasterizer_device_lost = halo::link::ref<uint8_t>(halo::main::vars().rasterizer_device_lost);
