/**
 * Timedemo benchmark update.
 */

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "interface.h"
#include "rasterizer.h"
#include "main.h"
#include <stdio.h>

#include "halo/main/timedemo.hpp"
#include "halo/effects/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/main/api.hpp"
#include "halo/rasterizer/api.hpp"

extern "C" { extern main_globals main_globals_data; }
extern "C" { extern timedemo_globals timedemo_globals_data; }
extern "C" { extern int32_t game_time_force_single_tick; }
extern "C" { extern int32_t timedemo_last_frame_index; }
extern "C" { extern uint8_t local_player_input_frozen[]; }
extern "C" { extern player_globals *local_player_globals; }
extern "C" { extern uint8_t console_debug_flag_5; }
extern "C" { extern char timedemo_pixel_shader_version[0x14]; }
extern "C" { extern int32_t os_platform_refresh_default; }
extern "C" { extern char *graphics_vendor_name; }
extern "C" { extern char *graphics_device_name; }
extern "C" { extern uint32_t graphics_device_id; }
extern "C" { extern uint16_t graphics_driver_version[4]; }
extern "C" { extern uint32_t physical_memory; }
extern "C" { extern uint32_t cpu_speed; }
extern "C" { extern uint32_t video_memory; }
extern "C" { extern char *shell_command_line; }
extern "C" { extern uint32_t shell_startup_tick_count; }
extern "C" { extern int32_t shell_nosound; }
extern "C" { extern int16_t sound_permutation_limit; }
extern "C" { extern uint8_t directsound_eax_enabled; }
extern "C" { extern int32_t directsound_quality; }
extern "C" { extern int16_t renderer_texture_quality; }
extern "C" { extern int16_t light_count_enabled; }
extern "C" { extern uint8_t console_debug_toggle_6893f2; }
extern "C" { extern uint8_t console_debug_toggle_6893fa; }
extern "C" { extern char hs_compile_and_evaluate(const char *command); }
extern "C" { extern uint32_t user_profile_signin_state_is_valid(void); }
namespace halo::main {

/**
 * Once per presented frame while -timedemo runs: samples the frame time into the timedemo
 * buckets, keeps the local player idle, and steps the benchmark script (a30 at frame 100, then
 * b30, c10 and d20 through hs map_name, and at frame 0x125c appends the report to
 * timedemo.txt and sets main_globals.quit). The frame counter advances on every call.
 *
 * @address 0x4c6f30
 */
void Timedemo::benchmark_update(void)
{
    uint32_t frame_time;
    int32_t step;
    FILE *file;
    char module_path[0x104];
    char date[0x20];
    char time[0x20];
    uint32_t version_size;
    uint32_t version_handle;
    void *version_data;
    uint32_t *fixed_file_info;
    const char *shader;
    const char *frame_suffix;
    double total_time;
    double frame_count;
    float slow_time;
    const char *sound_variety;
    const char *sound_quality;
    const char *environmental_sound;
    const char *hardware_acceleration;
    const char *texture_quality;
    const char *particles;
    const char *decals;
    const char *shadows;
    const char *specular;

    if (game_time_force_single_tick == 0) {
        return;
    }

    if (halo::rasterizer::globals().present_counter_low != timedemo_last_frame_index ||
        halo::rasterizer::globals().present_counter_high != (timedemo_last_frame_index >> 31)) {
        timedemo_last_frame_index = halo::rasterizer::globals().present_counter_low;
        timedemo_globals_data.current_time_ms = halo::cseries::time_query_performance_counter_ms();
        frame_time = timedemo_globals_data.current_time_ms - timedemo_globals_data.previous_time_ms;
        timedemo_globals_data.previous_time_ms = timedemo_globals_data.current_time_ms;
        timedemo_globals_data.frame_time_ms = frame_time;
        if (timedemo_globals_data.frame_count == 0) {
            frame_time = 1;
            timedemo_globals_data.frame_time_ms = 1;
        }
        timedemo_globals_data.frame_count++;
        timedemo_globals_data.total_time_ms += frame_time;
        if (frame_time > k_timedemo_below_60fps_ms) {
            timedemo_globals_data.buckets[0].time_ms += frame_time;
            timedemo_globals_data.buckets[0].frames++;
        }
        if (frame_time > k_timedemo_below_50fps_ms) {
            timedemo_globals_data.buckets[1].time_ms += frame_time;
            timedemo_globals_data.buckets[1].frames++;
        }
        if (frame_time > k_timedemo_below_40fps_ms) {
            timedemo_globals_data.buckets[2].time_ms += frame_time;
            timedemo_globals_data.buckets[2].frames++;
        }
        if (frame_time > k_timedemo_below_30fps_ms) {
            timedemo_globals_data.buckets[3].time_ms += frame_time;
            timedemo_globals_data.buckets[3].frames++;
        }
        if (frame_time > k_timedemo_below_25fps_ms) {
            timedemo_globals_data.buckets[4].time_ms += frame_time;
            timedemo_globals_data.buckets[4].frames++;
        }
        if (frame_time > k_timedemo_below_20fps_ms) {
            timedemo_globals_data.buckets[5].time_ms += frame_time;
            timedemo_globals_data.buckets[5].frames++;
        }
        if (frame_time > k_timedemo_below_15fps_ms) {
            timedemo_globals_data.buckets[6].time_ms += frame_time;
            timedemo_globals_data.buckets[6].frames++;
        }
        if (frame_time > k_timedemo_below_10fps_ms) {
            timedemo_globals_data.buckets[7].time_ms += frame_time;
            timedemo_globals_data.buckets[7].frames++;
        }
        if (frame_time > k_timedemo_below_5fps_ms) {
            timedemo_globals_data.buckets[8].time_ms += frame_time;
            timedemo_globals_data.buckets[8].frames++;
        }
    }

    step = game_time_force_single_tick;
    local_player_input_frozen[0] = 1;
    *((uint8_t *)local_player_globals + 0x11) = 1;
    console_debug_flag_5 = 1;

    switch (step) {
    case _timedemo_step_load_a30:
        halo::main::main_queue_map_change((char *)"a30");
        game_time_force_single_tick++;
        return;
    case _timedemo_step_load_b30:
        hs_compile_and_evaluate("map_name b30");
        break;
    case _timedemo_step_load_c10:
        hs_compile_and_evaluate("map_name c10");
        break;
    case _timedemo_step_load_d20:
        hs_compile_and_evaluate("map_name d20");
        break;
    case _timedemo_step_report:
        main_globals_data.quit = 1;
        file = (FILE *)fopen("timedemo.txt", "a");
        GetModuleFileNameA(0, module_path, 0x104);
        fseek(file, 0, SEEK_END);
        GetDateFormatA(0x400 , 0, 0, 0, date, 0x20);
        GetTimeFormatA(0x400, 0, 0, 0, time, 0x20);
        fprintf(file, "Date / Time: %s %s (%dms)\n", date, time, shell_startup_tick_count);

        if (halo::shell::globals().force_shader == 9999) {
            shader = "2.0a";
        } else if (halo::rasterizer::globals().caps.pixel_shader_version < 0xffff0101u) {
            shader = "Fixed Function";
        } else {
            sprintf(timedemo_pixel_shader_version, "%d.%d",
                (halo::rasterizer::globals().caps.pixel_shader_version >> 8) & 0xff,
                halo::rasterizer::globals().caps.pixel_shader_version & 0xff);
            shader = timedemo_pixel_shader_version;
        }
        if (graphics_device_id != 0) {
            fprintf(file, "%dMHz, %dMB, %dM %s %s (DeviceID=0x%04x) Driver=%d.%d.%d.%d Shader=%s\n",
                cpu_speed, physical_memory, video_memory >> 20, graphics_vendor_name,
                graphics_device_name, graphics_device_id,
                (uint32_t)graphics_driver_version[3], (uint32_t)graphics_driver_version[2],
                (uint32_t)graphics_driver_version[1], (uint32_t)graphics_driver_version[0], shader);
        } else {
            fprintf(file, "%dMHz, %dMB\n", cpu_speed, physical_memory);
        }

        fprintf(file, "%s %s", module_path, shell_command_line);
        version_size = GetFileVersionInfoSizeA(module_path, (LPDWORD)(&version_handle));
        version_data = GlobalAlloc(0, version_size);
        GetFileVersionInfoA(module_path, 0, version_size, version_data);
        VerQueryValueA(version_data, "\\", (void **)&fixed_file_info, &version_handle);
        fprintf(file, "   (Version=%d.%d.%d.%d)\n",
            fixed_file_info[2] >> 16, fixed_file_info[2] & 0xffff,
            fixed_file_info[3] >> 16, fixed_file_info[3] & 0xffff);
        GlobalFree(version_data);

        frame_suffix = timedemo_globals_data.buckets[8].frames == 1 ? ")" : "s)";
        total_time = (double)timedemo_globals_data.total_time_ms;
        slow_time = (float)timedemo_globals_data.buckets[8].time_ms;
        frame_count = (double)timedemo_globals_data.frame_count;
        fprintf(file,
            "Frames=%d\nTotal Time=%.2fs\nAverage frame rate=%.2ffps\n"
            "Below  5fps=% 2d%% (time)  %d%% (frames) (%.3fs spent in %d frame%s\n"
            "Below 10fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 15fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 20fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 25fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 30fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 40fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 50fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 60fps=% 2d%% (time)  %d%% (frames)\n",
            timedemo_globals_data.frame_count,
            total_time * 0.001,
            1000.0 / (total_time / frame_count),
            timedemo_globals_data.buckets[8].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[8].frames * 100 / timedemo_globals_data.frame_count,
            (double)(slow_time * 0.001f),
            timedemo_globals_data.buckets[8].frames,
            frame_suffix,
            timedemo_globals_data.buckets[7].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[7].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[6].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[6].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[5].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[5].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[4].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[4].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[3].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[3].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[2].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[2].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[1].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[1].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[0].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[0].frames * 100 / timedemo_globals_data.frame_count);

        if (shell_nosound != 0) {
            fprintf(file, "###Sound Options###\nSound Disabled\n");
        } else {
            if (sound_permutation_limit == 2) {
                sound_variety = "High";
            } else if (sound_permutation_limit == 1) {
                sound_variety = "Medium";
            } else {
                sound_variety = "Low";
            }
            if (directsound_eax_enabled != 0 && user_profile_signin_state_is_valid() != 0) {
                environmental_sound = "EAX";
            } else {
                environmental_sound = "No";
            }
            if (directsound_quality == 2) {
                sound_quality = "High";
            } else if (directsound_quality == 1) {
                sound_quality = "Normal";
            } else {
                sound_quality = "Low";
            }
            hardware_acceleration = directsound_eax_enabled == 1 ? "Yes" : "No";
            fprintf(file,
                "###Sound Options###\nHardware Acceleration= %s\nSound Quality= %s\n"
                "Environmental Sound= %s\nSound Variety= %s\n",
                hardware_acceleration, sound_quality, environmental_sound, sound_variety);
        }

        if (renderer_texture_quality == 0) {
            texture_quality = "High";
        } else if (renderer_texture_quality == 1) {
            texture_quality = "Medium";
        } else {
            texture_quality = "Low";
        }
        if (light_count_enabled == 0) {
            particles = "High";
        } else if (light_count_enabled == 1) {
            particles = "Low";
        } else {
            particles = "Off";
        }
        decals = halo::effects::globals().decals_for_all_responses != 0 ? "Yes" : "No";
        shadows = console_debug_toggle_6893f2 != 0 ? "Yes" : "No";
        specular = console_debug_toggle_6893fa != 0 ? "Yes" : "No";
        fprintf(file,
            "###Video Options###\nResolution= %d x %d\nRefresh rate= %d Hz\n"
            "Framerate throttle= No Vsync\nSpecular= %s\nShadows= %s\nDecals= %s\nParticles= %s\n"
            "Texture Quality= %s\n\nFor further information, please visit the timedemo FAQ at: "
            "http://halo.bungie.net/site/halo/features/hpcperformancefaq.html \n",
            halo::rasterizer::globals().present_parameters.back_buffer_width,
            halo::rasterizer::globals().present_parameters.back_buffer_height,
            os_platform_refresh_default, specular, shadows, decals, particles, texture_quality);
        fclose(file);
        break;
    default:
        break;
    }
    game_time_force_single_tick++;
}

}
