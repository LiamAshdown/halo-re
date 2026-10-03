/**
 * Bink movie playback and frame capture.
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
#include "bink.h"
#include "win32.h"
#include "rasterizer.h"
#include "shell.h"
#include <stdint.h> 

#include "halo/main/movie.hpp"
#include "halo/main/layout.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/core/link.hpp"
#include "halo/main/vars.hpp"

static auto &main_globals_data = halo::link::ref<main_globals>(halo::main::vars().main_globals_data);
extern "C" { extern void rasterizer_capture_and_present(const int16_t *tile, BitmapData *bitmap); }
extern "C" { extern void path_append_component(char *destination, const char *component); }
extern "C" { extern void path_remove_last_component(uint8_t *path); }
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
typedef int32_t (__stdcall *d3d_test_cooperative_level_fn)(void *device);

typedef int32_t (__stdcall *d3d_create_offscreen_plain_surface_fn)(void *device, uint32_t width,
    uint32_t height, uint32_t format, uint32_t pool, void **surface, void *shared_handle);

typedef int32_t (__stdcall *d3d_get_render_target_fn)(void *device, uint32_t index, void **surface);

typedef int32_t (__stdcall *d3d_stretch_rect_fn)(void *device, void *source, const void *source_rect,
    void *dest, const void *dest_rect, uint32_t filter);

typedef uint32_t (__stdcall *d3d_release_fn)(void *object);

typedef int32_t (__stdcall *d3d_lock_rect_fn)(void *surface, d3d_locked_rect *locked,
    const void *rect, uint32_t flags);

typedef int32_t (__stdcall *d3d_unlock_rect_fn)(void *surface);

namespace halo::main {

/**
 * Plays one Bink movie full screen at 640x480: each frame is decoded into an X8R8G8B8
 * offscreen surface and stretched onto the render target, then presented. Escape or space
 * (key down followed by key up) or the window close command ends playback early, as does the
 * movie reaching its last frame or 0x007196d4 becoming nonzero. A lost device pauses the movie;
 * once the device can be reset the surfaces are recreated and playback resumes with a full
 * frame copy.
 *
 * @address 0x43ed20
 */
void MoviePlayer::play_bink(const char *movie_path)
{
    void *offscreen_surface;
    void *render_target;
    uint8_t skip;
    uint8_t copy_all;
    uint8_t skip_key_down;
    bink_movie_prefix *bink;
    win32_msg message;
    d3d_locked_rect locked;
    d3d_present_parameters present_parameters;
    int32_t result;

    render_target = 0;
    offscreen_surface = 0;
    skip = 0;
    copy_all = 1;
    skip_key_down = 0;
    if (movie_playback_abort != 0) {
        return;
    }

    if (d3d9::device_function<d3d_create_offscreen_plain_surface_fn>(halo::rasterizer::globals().device, d3d9::device_method::create_offscreen_plain_surface)(
            halo::rasterizer::globals().device, k_movie_surface_width, k_movie_surface_height, d3d9::k_format_x8r8g8b8, 0,
            &offscreen_surface, 0) != 0) {
        return;
    }
    if (d3d9::device_function<d3d_get_render_target_fn>(halo::rasterizer::globals().device, d3d9::device_method::get_render_target)(
            halo::rasterizer::globals().device, 0, &render_target) != 0) {
        d3d9::surface_function<d3d_release_fn>(render_target, d3d9::surface_method::release)(render_target);
        return;
    }

    BinkSetSoundSystem((void *)BinkOpenDirectSound, 0);
    bink = (bink_movie_prefix *)(BinkOpen(movie_path, 0));
    if (bink != 0) {
        do {
            if (PeekMessageA((LPMSG)&message, 0, 0, 0, 1 ) != 0) {
                do {
                    TranslateMessage((const MSG *)&message);
                    if (message.message == win32::k_wm_keydown) {
                        if (message.wparam == win32::k_vk_escape || message.wparam == win32::k_vk_space) {
                            skip_key_down = 1;
                        }
                    } else if (message.message == win32::k_wm_keyup) {
                        if (skip_key_down != 0 && (message.wparam == win32::k_vk_escape || message.wparam == win32::k_vk_space)) {
                            skip = 1;
                        }
                    } else if (message.message == win32::k_wm_syscommand) {
                        if (message.wparam == win32::k_sc_close) {
                            skip = 1;
                        }
                    }
                    DispatchMessageA((const MSG *)&message);
                } while (PeekMessageA((LPMSG)&message, 0, 0, 0, 1) != 0);
                if (skip != 0) {
                    break;
                }
            }

            result = d3d9::device_function<d3d_test_cooperative_level_fn>(halo::rasterizer::globals().device, d3d9::device_method::test_cooperative_level)(
                halo::rasterizer::globals().device);
            if (result == d3d9::k_error_device_not_reset) {
                if (bink->paused == 0) {
                    BinkPause(bink, 1);
                }
                if (render_target != 0) {
                    d3d9::surface_function<d3d_release_fn>(render_target, d3d9::surface_method::release)(render_target);
                    render_target = 0;
                }
                if (offscreen_surface != 0) {
                    d3d9::surface_function<d3d_release_fn>(offscreen_surface, d3d9::surface_method::release)(offscreen_surface);
                    offscreen_surface = 0;
                }
                present_parameters = halo::rasterizer::globals().present_parameters;
                halo::rasterizer::rasterizer_device_reset(&present_parameters);
                rasterizer_device_lost = 0;
                d3d9::device_function<d3d_create_offscreen_plain_surface_fn>(halo::rasterizer::globals().device, d3d9::device_method::create_offscreen_plain_surface)(
                    halo::rasterizer::globals().device, k_movie_surface_width, k_movie_surface_height, d3d9::k_format_x8r8g8b8, 0, &offscreen_surface, 0);
                d3d9::device_function<d3d_get_render_target_fn>(halo::rasterizer::globals().device, d3d9::device_method::get_render_target)(
                    halo::rasterizer::globals().device, 0, &render_target);
            } else if (result != 0) {
                if (bink->paused == 0) {
                    BinkPause(bink, 1);
                }
            } else {
                if (bink->paused != 0) {
                    BinkPause(bink, 0);
                    copy_all = 1;
                }
                if (BinkWait(bink) == 0 && offscreen_surface != 0 && render_target != 0) {
                    BinkDoFrame(bink);
                    if (d3d9::surface_function<d3d_lock_rect_fn>(offscreen_surface, d3d9::surface_method::lock_rect)(
                            offscreen_surface, &locked, 0, 0) == 0) {
                        BinkCopyToBuffer(bink, (void *)(uintptr_t)locked.bits, locked.pitch, k_movie_surface_height, 0, 0,
                            (copy_all != 0 ? k_bink_copy_all : 0) + k_bink_copy_format_32bit);
                        copy_all = 0;
                        d3d9::surface_function<d3d_unlock_rect_fn>(offscreen_surface, d3d9::surface_method::unlock_rect)(offscreen_surface);
                    }
                    BinkNextFrame(bink);
                    d3d9::device_function<d3d_stretch_rect_fn>(halo::rasterizer::globals().device, d3d9::device_method::stretch_rect)(
                        halo::rasterizer::globals().device, offscreen_surface, 0, render_target, 0, 0);
                    halo::rasterizer::rasterizer_capture_and_present(0, 0);
                }
            }
        } while (bink->frame_index != bink->frame_count && movie_playback_abort == 0);
        BinkClose(bink);
    }
    d3d9::surface_function<d3d_release_fn>(offscreen_surface, d3d9::surface_method::release)(offscreen_surface);
    d3d9::surface_function<d3d_release_fn>(render_target, d3d9::surface_method::release)(render_target);
}

}

#undef D3D_VTABLE
